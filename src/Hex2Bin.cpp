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

#include <err.h>	/* err */
#include <stdio.h>	/* fopen, fgetln, fputs, fwrite */
#include <stdlib.h>
#include <string.h>

int HexDataSize; 

//   (char *[string to split], int [position ob byte], int [Nu,ber of bytes to extract] 
char* ToByte(char *HexData, int SelectedByte, int NumberofBytes ) 
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

int GetChecksum (char *HexData, int LineLength)
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

int Complement (int Checksum)
{
 	return ((0-Checksum) & 0x000000FF);
}

int* VerifyFile(char *HexFileName)
{
	FILE *HexFile=NULL;
	unsigned int HexFileSize;
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
