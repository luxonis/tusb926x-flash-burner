/*

MIT License

Copyright (c) 2022 Luxonis LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <err.h>

#include <sys/ioctl.h>
 #include <fcntl.h>

#include <hidapi/hidapi.h>
#include "../include/DescriptorDeclarations.h"

#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define MAX_STR 255

#ifndef HID_API_MAKE_VERSION
#define HID_API_MAKE_VERSION(mj, mn, p) (((mj) << 24) | ((mn) << 8) | (p))
#endif
#ifndef HID_API_VERSION
#define HID_API_VERSION HID_API_MAKE_VERSION(HID_API_VERSION_MAJOR, HID_API_VERSION_MINOR, HID_API_VERSION_PATCH)
#endif

#if defined(USING_HIDAPI_LIBUSB) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_libusb.h>
#endif

#define HID_REPORT_LEN 9u

typedef unsigned char uchar;

#define TUSB9260_VENDOR_ID 0x0451u
#define TUSB9260_PRODUCT_ID 0x926Bu


#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 1)   /* endpoint address */
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2)   /* endpoint address */

#define USB_TIMEOUT	        50000        /* Connection timeout (in ms) */

static bool GetOutBuffer (void);

static libusb_device_handle* get_usb_device(libusb_context **ctx);
static int write_usb_bulk_data(libusb_device_handle* handle, uint8_t* inBuff, uint32_t size, int* actualLen);

bool UAS_descriptorsFlag= false;
bool HID_descriptorsFlag= false;
bool Self_powered_flag= false;
bool RemoteWakeup_flag= false;
bool DescriptorsWindow_Flag= false;
char* ManufactureID=NULL;
char* VID=NULL;
char* PID=NULL;
char* ProductID=NULL;
char* SerialID=NULL;
char* Ex_SerialID=NULL;
bool ExtDescriptors_flag=false;
bool allflag=false;
bool UseSpeciicSerialNumber = false;
bool UseDieIDinSerNum = true;
bool Bin = false;
int* HexFWBuffer=NULL;

uchar ID_hex[4];

char *BinName=NULL;
char *HexName=NULL;

Output_Buffer OutBuffer;

static libusb_device_handle* get_usb_device(libusb_context **ctx)
{
	libusb_device_handle *dev_handle = NULL;

	int rc = libusb_init(ctx);
	if(rc < 0) {
		printf("Failed to initialize libusb context wit error code: %d\n", rc);
	}

	libusb_set_option(*ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

	dev_handle = libusb_open_device_with_vid_pid(*ctx, TUSB9260_VENDOR_ID, TUSB9260_PRODUCT_ID);
	if (!dev_handle) {
		printf("Error finding TUSB9260 device\n");
	}
	return dev_handle;
}

static int write_usb_bulk_data(libusb_device_handle* handle, uint8_t* inBuff, uint32_t size, int* actualLen){
	int ret = -1;
    ret = libusb_bulk_transfer(handle, USB_ENDPOINT_OUT, inBuff, size,
		actualLen, USB_TIMEOUT);
      //Error handling
      switch(ret){
          case 0:
          printf("Sent %d bytes to device\n", *actualLen);
          ret = 0;
          break;
      case LIBUSB_ERROR_TIMEOUT:
          printf("ERROR in bulk write: %d Timeout\n", ret);
          break;
      case LIBUSB_ERROR_PIPE:
          printf("ERROR in bulk write: %d Pipe\n", ret);
          break;
      case LIBUSB_ERROR_OVERFLOW:
          printf("ERROR in bulk write: %d Overflow\n", ret);
          break;
      case LIBUSB_ERROR_NO_DEVICE:
          printf("ERROR in bulk write: %d No Device\n", ret);
          break;
      default:
          printf("ERROR in bulk write: %d\n", ret);
          break;
    }

	return ret;
}

static char* ToByte(char *HexData, int SelectedByte, int NumberofBytes ) 
{	
	int index;
	char *ByteData;
	int value;

	ByteData =new char [NumberofBytes*2];
	for (index=1; index < ((NumberofBytes *2) +1); index++)
	{
	  ByteData[index-1] = HexData[(SelectedByte*2)+index];
	  //printf("Bytedata[%d]= HexData[%d]\n",index-1,(SelectedByte*2)+index);
	}
	ByteData[NumberofBytes*2]='\0';
	//printf("BYTE= %s\n",ByteData);
	
  return ByteData; 	
}

static int Complement (int Checksum)
{
 	return ((0-Checksum) & 0x000000FF);
}

static int GetChecksum (char *HexData, int LineLength)
{
 	int offset;
 	int Checksum =0;
 	int DataByte;

 	for (offset=0; offset < LineLength; offset++)
 	{	
  	 sscanf(ToByte(HexData,0+offset,1), "%x", &DataByte);
  	 Checksum += DataByte;
  	 Checksum = Checksum & 0x00FF;
 	}
 	
	return Checksum;
}


static int* VerifyFile(const char *HexFileName)
{
	FILE *HexFile=NULL;
	uint HexFileSize;
	int HexLineLength;
	int MaxHeaderLength=16;
	char *HexLine=NULL;
	int ByteCount;
	char *substring= NULL;
 	int index = 0;
	
	int CurrentAddress;
	int RecordType;	
	int Checksum;
	int BufferIndex;
	int *HexFirmwareBuffer=NULL;

	//debug printf("\nValidating file %s\n", HexFileName);	

	HexFile = fopen(HexFileName, "r");
	if (HexFile == NULL)
		err(1, HexFileName);

	fseek (HexFile, 0, SEEK_END);
 	HexFileSize =ftell(HexFile);
 	fseek (HexFile, 0, SEEK_SET);
  
        HexLine = (char *) malloc (HexFileSize);
	HexFirmwareBuffer = (int *) malloc (HexFileSize+1);
	fgets(HexLine, MaxHeaderLength, HexFile); //Verify header file
		
	HexLine[MaxHeaderLength]='\0';
	if (strcmp(HexLine,":020000040800F2") && strcmp(HexLine,":020000040000FA"))
	 {          
	  printf("The selected .hex file is not valid.");
	  fclose(HexFile);
	 free(HexLine);
	 return HexFirmwareBuffer;
	 } 

	HexDataSize = 0;
	BufferIndex = 0;
	while (fgets(HexLine, HexFileSize, HexFile) != NULL) 
 	{
         index++;
	 for (HexLineLength=0; HexLineLength < HexFileSize; HexLineLength++)
	 {
	   if (HexLine[HexLineLength] =='\n')
	   {
            HexLine[HexLineLength-1]='\0';	    
	    break;
 	   }
         }
         
	 if (!strcmp(HexLine,":00000001FF"))
         {
	  break;
	 }	 
	 
	 if (HexLine[0] == ':')
         {
          if (HexLineLength >= 13 )
	  {
	    sscanf(ToByte(HexLine,0,1), "%x", &ByteCount);  //capture and verify size
	    if (HexLineLength-1 != ((ByteCount*2)+11))
	    {
	      printf ("Invalid File. Line lenght mismatch on line: %d\n", index);
	      break;
	    }

	    sscanf(ToByte(HexLine,1,2), "%x", &CurrentAddress); //capture and verify address
   	    if (CurrentAddress != HexDataSize)
	    {
	      printf ("Invalid File. MemoryAddresses are not continuous. (Line %d ).", index);
	      break;
	    }
	    

	    sscanf(ToByte(HexLine,3,1), "%x", &RecordType); //capture and verify RecordType
	    if (RecordType != 0)
	    {
	      printf("Invalid File. Unsupported record type. (Line %d ).", index);
	      break;
 	    }
	      	    
	    sscanf(ToByte(HexLine,ByteCount+4,1), "%x", &Checksum); //capture the line's Checksum
	    if (Complement(GetChecksum(HexLine, ByteCount + 4)) != Checksum)
	    {
	      printf("Invalid File. Checksum at line %d does not match.", index);
	      break;
	    }
	    	    	    	    
	    //capture all line's data
	    
	    for (BufferIndex=0; BufferIndex < ByteCount;BufferIndex++)
	    {
	      sscanf(ToByte (HexLine, BufferIndex+4, 1), "%x", &HexFirmwareBuffer [HexDataSize+BufferIndex] );                 
	    }
	     	    
	    HexDataSize += ByteCount;	    
	   	                
	  }
         }         
	}

	fclose(HexFile);
	free(HexLine);
	return HexFirmwareBuffer;
}

static void getIDs (char ID [4])
{
  uint 	ID_size;
    
  for ( ID_size = 0; ID_size < 4; ID_size ++ )
  {
   if ( ID_size == 0 || ID_size == 2 )
       {
	   if(ID[ID_size] > 0x60)
	   {
	       ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x57) << 4 ) ;
	   }
	   else
	   {
	       if ( ID[ID_size] > 0x39 )
	       {
		   ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x37) << 4 ) ;
	       }
                       else
	       {
		   ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x30) << 4 );
	       }
	   }
       }
   else
       {
	   if(ID[ID_size] > 0x60)
	   {
	       ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x37) ) ;
	   }
	   else
	   {
	       if ( ID[ID_size] > 0x39 )
	       ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x37) ) ;
	       else
	       {
		   ID_hex[ID_size] = (uchar) (((ID[ID_size]) - 0x30) );
	       }
	   }      
       }
  }
 
}

void print_device(struct hid_device_info *cur_dev) {
	printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
	printf("\n");
	printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
	printf("  Product:      %ls\n", cur_dev->product_string);
	printf("  Release:      %hx\n", cur_dev->release_number);
	printf("  Interface:    %d\n",  cur_dev->interface_number);
	printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
	printf("\n");
}

void print_devices(struct hid_device_info *cur_dev) {
	while (cur_dev) {
		print_device(cur_dev);
		cur_dev = cur_dev->next;
	}
}

static bool ValidHex (char *CustomSerialNumber, int SerialLength)
{
  for (int SerialIteration= 0; SerialIteration< SerialLength; SerialIteration++ ) {
    if ( (CustomSerialNumber[SerialIteration] < 0x30) || ((CustomSerialNumber[SerialIteration] > 0x39) && (CustomSerialNumber[SerialIteration] < 0x41)) || ((CustomSerialNumber[SerialIteration] > 0x46) && (CustomSerialNumber[SerialIteration] < 0x61)) || (CustomSerialNumber[SerialIteration] < 0x66) ){      
      return false;
    }
  }
  printf("Valid Hex = %s\n",CustomSerialNumber);
  return true;	
}

static int send_reprogramming_enable_cmd(hid_device *dev)
{
	const uint8_t reprogr_enable_cmd[HID_REPORT_LEN] = {0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
 	int res = -1;
	printf("Issuing USB_HID_ENABLE_REPROGRAM command\n");
	if(HID_REPORT_LEN == hid_write(dev, reprogr_enable_cmd, HID_REPORT_LEN)){
		printf("USB_HID_ENABLE_REPROGRAM sent successfuly\n");
		res = 0;
	}
	else{
		printf("USB_HID_ENABLE_REPROGRAM command failed: %ls\n", hid_error(dev));
	}
	return res;
}

static int send_poison_flash_cmd(hid_device *dev){
	const uint8_t poison_flash_cmd[HID_REPORT_LEN] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int res = -1;
	if(HID_REPORT_LEN == hid_write(dev, poison_flash_cmd, HID_REPORT_LEN)){
		printf("USB_HID_POISON_FLASH sent successfully\n");
		res = 0;
	}
	else{
		printf("Failed to send USB_HID_POISON_FLASH command: %ls\n", hid_error(dev));
	}
	return res;
}

static int send_reset_device_cmd(hid_device *dev){
	const uint8_t reset_dev_cmd[HID_REPORT_LEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int res = -1;
	if(HID_REPORT_LEN == hid_write(dev, reset_dev_cmd, HID_REPORT_LEN)){
		printf("USB_HID_RESET_FLASH_BURNER_DEVICE sent successfully\n");
		res = 0;
	}
	else{
		printf("Failed to send USB_HID_RESET_FLASH_BURNER_DEVICE command: %ls\n", hid_error(dev));
	}
	return res;
}

static int send_setup_download_cmd(hid_device *dev)
{
	const uint8_t setup_download_cmd[HID_REPORT_LEN] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int res = -1;
	if(HID_REPORT_LEN == hid_write(dev, setup_download_cmd, HID_REPORT_LEN)){
		printf("USB_HID_SETUP_DOWNLOAD_DATA sent successfully\n");
		res = 0;
	}
	else{
		printf("Failed to send USB_HID_SETUP_DOWNLOAD_DATA command: %ls\n", hid_error(dev));
	}
	return res;
}

static hid_device* get_hid_device(void){
  hid_device *dev = hid_open(TUSB9260_VENDOR_ID, TUSB9260_PRODUCT_ID, NULL);
  if(NULL == dev){
    printf("Failed to open HID device: %ls\n", hid_error(dev));
  }
  return dev;
}


static int erase_flash(void)
{
  hid_device* dev = NULL;
	int result = -1;
		do{
      dev = get_hid_device();
      if(NULL  == dev)
        break;
			if(send_reprogramming_enable_cmd(dev))
				break;
			if(send_poison_flash_cmd(dev))
				break;

      sleep(4);

			if(send_reset_device_cmd(dev)){
				result = 0;
				break;
			}
      hid_close(dev);
		}while(0);
    /* Free static HIDAPI objects. */
	hid_exit();
	return result;
}

int program_flash(void)
{
  int result = -1;
  int actual_len = 0;
  hid_device *dev = NULL;
  libusb_context *usb_ctx = NULL;
  libusb_device_handle* usb_handle = NULL;

  do{
    dev = get_hid_device();
    if(NULL  == dev)
      break;
    if(send_reprogramming_enable_cmd(dev))
      break;
    if(send_poison_flash_cmd(dev))
      break;

    sleep(4);

    if(send_reset_device_cmd(dev)){
      break;
    }
    hid_close(dev);

    sleep(4);

    dev = get_hid_device();
    if(NULL  == dev)
      break;

    if (!GetOutBuffer()) {
      printf("ERROR: GetOutBuffer FAIL!!!");
    break;
    }
    if(send_setup_download_cmd(dev))
      break;

    sleep(4);
    
    usb_handle = get_usb_device(&usb_ctx);
    if(NULL == usb_handle){
      printf("Failed to get usb device\n");
      break;
    }

    if(libusb_kernel_driver_active(usb_handle, 0) == 1){    
        printf("\nKernel Driver Active");    
        if(libusb_detach_kernel_driver(usb_handle, 0) == 0)    
            printf("\nKernel Driver Detached!\n");    
        else    
        {    
            printf("\nCouldn't detach kernel driver!\n");    
            break;    
        }
    }

    result = libusb_claim_interface(usb_handle, 0);
    if(result)    {
      printf("Failed to claim usb device interface: %d\n", result);
      break;
    }

    if(write_usb_bulk_data(usb_handle, OutBuffer.Buffer, OutBuffer.BufferSize, &actual_len)){
        if(actual_len != OutBuffer.BufferSize){
          printf("ERROR in bulk write! expected size: %d, actual:%d\n", OutBuffer.BufferSize, actual_len);
          break;
      }
    }
    
    if(send_reset_device_cmd(dev)){
      break;
    }
    result = 0;
  }
	while(0);

  if(NULL != dev){
    hid_close(dev);
  }
  if(NULL != usb_handle){
      libusb_release_interface(usb_handle, 0);
      libusb_close(usb_handle);
      libusb_exit(usb_ctx);
  }
  /* Free static HIDAPI objects. */
	hid_exit();

	return result;
}

uchar Checksum( uchar* Data, int Lentgh)
{
 int index;
 uchar ChecksumData = 0x00;
 
 for ( index = 0 ; index < Lentgh ; index ++)
 {
	ChecksumData += Data[index];
 }

 return ChecksumData;
}

void Gen_serial_number() //DeviceListBox_activated
{    
   time_t rawtime;
   struct tm* timeinfo;
   char buffer [80];
   long int index,y;
      
   //FILE *SystemCommand;
   //char CommandValue[1024];
   char *pch;
   
   
   SerialID=new char [64];
   SerialID[63] = '\0';
   //SystemCommand = popen("hdparm -I /dev/sda | grep -i serial", "r");
   //fgets(CommandValue,sizeof(CommandValue), SystemCommand);
   //pclose(SystemCommand);
   //pch = strtok (CommandValue, " ");
   //pch = strtok (NULL, " ");
   //pch = strtok (NULL, " ");
   if(!ExtDescriptors_flag)
  {
   for (index=0; index<8; index++)
   {	
     y = (char) ((rand() % 10)+48);	
     SerialID [index]= y;
   }
   }
   SerialID [0]= '\0';
   
   time( &rawtime);
   timeinfo = localtime (&rawtime);

   strftime (buffer, 80, "%Y%m%d%H%M%S%j",timeinfo);
   if(!ExtDescriptors_flag)
  {
   strcat(SerialID,buffer);
   //strcat(SerialID,pch);
   }
   else
   {
   strcat(SerialID,Ex_SerialID);
   }
   //SWAT debugg printf("serial =%s size=%d\n", SerialID, strlen ( SerialID ));
   
}

static bool GetOutBuffer (void)
{
  
  uchar Descriptor_Header[3] = { 0x60, 0x92, 0x01 };
  uchar Firmware_Header [1] = {0x02};

  uint 	index,TextIndex;
  uchar* VID_hex;
  uchar* PID_hex;

  int status;
  const uint MaxDescriptorSize = 540;
  
  uchar* Descriptor_Data=NULL;
  uchar* Manufacture_Data=NULL; 
  char*  Manufacture_Text=NULL;
  uchar* Product_Data=NULL; 
  char*  Product_Text=NULL;
  uchar* Serial_Data=NULL; 
  char*  Serial_Text=NULL;

  uchar  DescriptorChecksum [1] = {0x00};
  uchar  FirmwareChecksum [1] = {0x00};

  uint 	 Int_DescriptorSize;
  char 	Descriptor_Size [4]={0x00,0x00,0x00,0x00}; 
  uchar* FirmwareBuffer = NULL;
  uint 	 Int_FirmwareSize;
  char	 Firmware_Size [4] = {0x00,0x00,0x00,0x00};

  FILE* FirmwareFile;
  //FILE*	OutFile; //only for testing
  
  
  //Replace the Device Descriptor VID with the one defined by the user.
  if(!ExtDescriptors_flag)
  {
   VID=new char [4];
   status = sprintf(VID,"0451");
  }
  getIDs (VID);
  VID_hex = ID_hex;

  Device_Descriptor[8] = VID_hex[2] + VID_hex[3];
  Device_Descriptor[9] = VID_hex[0] + VID_hex[1];

  //Replace the Device Descriptor PID with the one defined by the user.
  if(!ExtDescriptors_flag)
  {
   PID=new char [4];
   status = sprintf(PID,"9261");
   }
   getIDs (PID);
   PID_hex = ID_hex;
   
   Device_Descriptor[10] = PID_hex[2] + PID_hex[3];
   Device_Descriptor[11] = PID_hex[0] + PID_hex[1];

   //Get the Manufacture info and build the correspondant string descriptor.//SWAT:Change "ManufactureID" for dinamic string
  if(!ExtDescriptors_flag)
  {
   ManufactureID=new char [17];
   status = sprintf(ManufactureID,"Texas Instruments");
  }
   Manufacture_Data = new uchar [( (strlen ( ManufactureID )) * 2 ) +2];
   Manufacture_Text = new char [(strlen ( ManufactureID ))]; 
   Manufacture_Data[0] = ( (strlen (ManufactureID)) * 2 ) +2;
   Manufacture_Data[1] = 0x03;
   
   strncpy(Manufacture_Text,ManufactureID, strlen( ManufactureID ));
   index = 2;

   for ( TextIndex = 0; TextIndex < (strlen ( ManufactureID )); TextIndex ++)
    {
	Manufacture_Data[index] = (uchar) Manufacture_Text[TextIndex];
	Manufacture_Data[index + 1] = 0x00;
	index += 2;
    }

   //Get the Product info and build the correspondant string descriptor.
   
  if(!ExtDescriptors_flag)
  {
   ProductID=new char [22];
   status = sprintf(ProductID,"TUSB9260 USB 3 to SATA");
   }
   Product_Data = new uchar [( (strlen ( ProductID )) * 2 ) +2];
   Product_Text = new char [(strlen ( ProductID ))]; 
   Product_Data[0] = ( (strlen ( ProductID )) * 2 ) +2;
   Product_Data[1] = 0x03;

   strncpy(Product_Text,ProductID, strlen ( ProductID ) );
   index = 2;

   for ( TextIndex = 0; TextIndex < (strlen ( ProductID )); TextIndex ++)
    {
	Product_Data[index] = (uchar) Product_Text[TextIndex];
	Product_Data[index + 1] = 0x00;
	index += 2;
    }
 
   //Get the Serial info and build the correspondant string descriptor.
   if (!UseSpeciicSerialNumber)
   {
     Gen_serial_number();
   }

   Serial_Data = new uchar [( (strlen ( SerialID )) * 2 ) +2];
   Serial_Text = new char [(strlen ( SerialID ))]; 
   Serial_Data[0] = ( (strlen ( SerialID )) * 2 ) +2;
   Serial_Data[1] = 0x03;
   
   strncpy(Serial_Text,SerialID, strlen ( SerialID ) );
   index = 2;
   
   for ( TextIndex = 0; TextIndex < (strlen ( SerialID )); TextIndex ++)
    {
	Serial_Data[index] = (uchar) Serial_Text[TextIndex];
	Serial_Data[index + 1] = 0x00;
	index += 2;
    }

   //User option to keep the provided serial number without using the device's DieID.
   //Replace the first null character on the string with 0xCA
 
   if (!UseDieIDinSerNum)
    {
      Serial_Data[1] = 0xCA;
    }
  
   //Now we have user defined strings, we can calculate the size of the descriptor
   Int_DescriptorSize =sizeof ( Device_Descriptor )+
		      Manufacture_Data [0] +
		      Product_Data[0]+
		      Serial_Data[0] +
		      sizeof ( BOT_Descriptors);
 
   if (UAS_descriptorsFlag)
   {
	Int_DescriptorSize = Int_DescriptorSize + sizeof (UAS_Descriptors);
   }
   if (HID_descriptorsFlag)
   {
	Int_DescriptorSize = Int_DescriptorSize + sizeof (HID_Descriptors);
   }   
  
   //And now we create an array to store all the other temporary arrays that form the whole descriptor.           
    Descriptor_Data = new uchar [MaxDescriptorSize];
 
    memcpy (Descriptor_Data, Device_Descriptor, sizeof ( Device_Descriptor ));
    memcpy (Descriptor_Data + sizeof ( Device_Descriptor), Manufacture_Data, Manufacture_Data [0] );
    memcpy (Descriptor_Data + sizeof ( Device_Descriptor) + Manufacture_Data [0], Product_Data, Product_Data [0]);
    memcpy (Descriptor_Data + sizeof ( Device_Descriptor) + Manufacture_Data [0] + Product_Data [0], Serial_Data, Serial_Data [0]);
    memcpy (Descriptor_Data + sizeof ( Device_Descriptor) + Manufacture_Data [0] + Product_Data [0] + Serial_Data [0], BOT_Descriptors, sizeof ( BOT_Descriptors ));
    
    if (UAS_descriptorsFlag)
    {
	memcpy (Descriptor_Data + sizeof ( Device_Descriptor) + Manufacture_Data [0] + Product_Data [0] + Serial_Data [0] + sizeof(BOT_Descriptors),UAS_Descriptors, sizeof ( UAS_Descriptors ));	
    }
 
    if (UAS_descriptorsFlag)
    {
	memcpy (Descriptor_Data + sizeof ( Device_Descriptor) + Manufacture_Data [0] + Product_Data [0] + Serial_Data [0] + sizeof(BOT_Descriptors)+sizeof(UAS_Descriptors),HID_Descriptors, sizeof (HID_Descriptors ));	
    }
 
    for ( index = Int_DescriptorSize -1; index < MaxDescriptorSize; index++ )
    {
	    Descriptor_Data[index] = 0x00;	
    }

   //And we get the Descriptor's Checksum    
 
   DescriptorChecksum[0] = Checksum (Descriptor_Data, Int_DescriptorSize);
 
   if (Bin)
   {
   //Now we will read the firmware image into a buffer.
   if (BinName ==NULL)
    {
	printf("Please select a valid Firmware file.\n");
	return false;
    }

   FirmwareFile = fopen(BinName, "rb");
   if (!FirmwareFile)
   {
   	printf("*Unable to open the Firmware file.\n");
        return false;
   }
   fseek (FirmwareFile, 0, SEEK_END);
   Int_FirmwareSize =ftell(FirmwareFile);
   fseek (FirmwareFile, 0, SEEK_SET);
    
   FirmwareBuffer = (uchar *) malloc (Int_FirmwareSize+1);
   if( !fread (FirmwareBuffer , Int_FirmwareSize , 1 , FirmwareFile))
   {
       fclose(FirmwareFile);
       printf("Unable to read from the Firmware file.\n");
       return false;
    }
    fclose(FirmwareFile);
   }
   else
   {
     Int_FirmwareSize = (uint) HexDataSize;
     FirmwareBuffer = (uchar *) malloc (Int_FirmwareSize+1);
     memcpy(FirmwareBuffer, HexFWBuffer, Int_FirmwareSize);/////////
   }

   //And we get the Firmware's Checksum 
    for ( index = 0 ; index < Int_FirmwareSize ; index ++)
    {
	FirmwareChecksum[0] = FirmwareChecksum[0] + FirmwareBuffer[index];
    }
    
    //Get a byte array containing the Descriptor lenght.
    for(index =0; index < 4; index ++ )
    {	
	Descriptor_Size[index] = MaxDescriptorSize >> (index*8);
    }
    //Get a byte array containing the Firmware lenght.
    for(index =0; index < 4; index ++ )
    {	
	Firmware_Size[index] = Int_FirmwareSize >> (index*8);
    }

   //Finally, we have all the data we need, so we set the Output buffer to contain all of the data we'll later send to the device through an IOCTL call.
    OutBuffer.Buffer = new uchar [ 7 + MaxDescriptorSize + 6  + Int_FirmwareSize + 1];    
    OutBuffer.BufferSize = 7 + MaxDescriptorSize + 6  + Int_FirmwareSize + 1;

    uchar  tempo [1] = {0x02};
    memcpy ( OutBuffer.Buffer, Descriptor_Header ,  3);
    memcpy ( OutBuffer.Buffer + 3, Descriptor_Size, 4 );
    memcpy ( OutBuffer.Buffer + 7, Descriptor_Data, Int_DescriptorSize );
    memcpy ( OutBuffer.Buffer + 7 + MaxDescriptorSize, DescriptorChecksum, 1);
    memcpy ( OutBuffer.Buffer + 7 + MaxDescriptorSize + 1, tempo , 1);
    memcpy ( OutBuffer.Buffer + 7 + MaxDescriptorSize + 2, Firmware_Size, 4);
    memcpy ( OutBuffer.Buffer + 7 + MaxDescriptorSize + 6, FirmwareBuffer, Int_FirmwareSize);
    memcpy ( OutBuffer.Buffer + 7 + MaxDescriptorSize + 6 + Int_FirmwareSize, FirmwareChecksum, 1);
   
   //for testing purposes only: We write the data into a file so we can check it out.
   
/*   OutFile = fopen ( "Descriptor-Firmware.bin" , "wb" );
    if (OutFile == NULL)
    {
	printf("Open fail");
	return false;
    }
    if (!fwrite ( OutBuffer.Buffer , 1 , 7 + MaxDescriptorSize + 6  + Int_FirmwareSize +1 , OutFile ))
    {
	printf("Unable to write to the output file.");
    }
    fclose( OutFile );USB9260_0
*/  

//free(FirmwareBuffer);

delete[] ManufactureID;

delete[] Manufacture_Data;

delete[] Manufacture_Text;

delete[] VID;

delete[] PID;

delete[] ProductID;

delete[] Product_Data;

delete[] Product_Text;

delete[] SerialID;

delete[] Serial_Data;

delete[] Serial_Text;

return true;

}

static void setup_hid_library(void){
	if (hid_init()){
    printf("Failed to initialize hid library\n");
    exit(-1);
  }
}

static void list_hid_devices(void){
    hid_device *handle;
    struct hid_device_info *devs;
    int res;
	  wchar_t wstr[MAX_STR];
    //Enumerate all devices matching VID PID
    devs = hid_enumerate(TUSB9260_VENDOR_ID, TUSB9260_PRODUCT_ID);
    print_devices(devs);
    hid_free_enumeration(devs);
}

int main(int argc, char* argv[])
{
  ////TEST
  char *CustomSerialNumber =NULL;
  char* SerialID=NULL;
  char* Ex_SerialID=NULL;
  bool UseSpeciicSerialNumber = false;
  bool UseDieIDinSerNum = true;
  const int MaxFirmwareFileSize=65535;
  int BinSize; 
  struct stat BinProperties;
  char* FWfile = NULL;
  int result = -1;
  ////
  setup_hid_library();
  list_hid_devices();

  //usage info
  if(1 == argc || ((2 == argc) && !strncmp(argv[1], "-h", sizeof("-h")))){
      printf("Usage: %s -f {flash_image}\n", argv[0]);
      return 0;
  }
  //erase flash
  else if((2 == argc) && !strncmp(argv[1], "-e", sizeof("-e"))){
    return erase_flash();
  }
  //reprogram
  else if(((3 == argc) && !strncmp(argv[1], "-f", sizeof("-f")))){
    FWfile = strrchr(argv[2], '.');
    //.bin file
    if(!strcmp(FWfile,".bin"))
    {
      BinName = argv[2];
      stat(BinName, &BinProperties);
      BinSize = BinProperties.st_size;
      
      if(BinSize > MaxFirmwareFileSize)
      {
        printf("\nFirmware file too long. The firmware content must be under %d bytes\n.", MaxFirmwareFileSize);
        return 1;
      }
      printf("Using binary file: %s size: %ld\n", BinName, BinProperties.st_size);
      Bin =true;

      return program_flash();
    }
    //.hex file
    else if(!strcmp(FWfile,".hex")){
    char* HexName=argv[2];
    HexFWBuffer=VerifyFile(HexName);    
  
    if(HexDataSize > MaxFirmwareFileSize)
    {
      printf("\nFirmware file too long. THe firmware content mus be under %d bytes\n.", MaxFirmwareFileSize);
      return 1;
    }
      return program_flash();
   }
  }
  else{
    printf("Usage: \"%s -f {flash_image}\" to flash the device or\n", argv[0]);
    printf("\"%s -e to erase the device\n", argv[0]);
  }
	return 0;
}
