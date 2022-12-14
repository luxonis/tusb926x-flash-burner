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

#ifndef _DESCRIPTORDECLARATIONS_H_
#define _DESCRIPTORDECLARATIONS_H_

#include <sys/types.h>
#include <stdint.h>

#undef USB20_ONLY
#define BOT_ENABLE
#define UAS_ENABLE
#define HID_ENABLE

#define USB_DT_DEVICE                     			0x01
#define USB_DT_CONFIG                    			0x02
#define USB_DT_STRING                    			0x03
#define USB_DT_INTERFACE                 			0x04
#define USB_DT_ENDPOINT                  			0x05
#define USB_DT_DEVICE_QUALIFIER          			0x06
#define USB_DT_OTHER_SPEED_CONFIG        		0x07
 #define USB_DT_INTERFACE_POWER           			0x08
#define USB_DT_OTG                       				0x09
#define USB_DT_DEBUG                     			0x0A
#define USB_DT_INTERFACE_ASSOCIATION     		0x0B
#define USB_DT_BOS                       				0x0F
#define USB_DT_DEVICE_CAPABILITY         			0x10
#define USB_DT_HID                       				0x21
#define USB_DT_REPORT                    			0x22
#define USB_DT_PHYSICAL_DESCRIPTOR       		0x23   /* Not used */
#define USB_DT_PIPE_USAGE                			0x24
#define USB_DT_SUPERSPEED_ENDPT_COMPANION  	0x30

#define UMS_INTERFACE_NUM    				0x00
#define MAX_BURST_SIZE        				0x07 // 0 represents 1 burst.
#define MAX_STREAMS           				0x04 /* Num of streams = 2^(val) - should match up w/ NCQ depth */
#define HID_INTERFACE_NUM    				0x01

extern int HexDataSize; 
#define TOTAL_NUM_INTERFACES 				0x01
typedef unsigned char uchar;
uchar  bMaxPower = 0x0D; 


 uchar Device_Descriptor [32] = 
 { 
     /* Device Descriptor */
     0x12,                       /* bLength              */
     USB_DT_DEVICE,              /* DEVICE               */
     0x00,0x03,                  /* USB 3.0              */
     0x00,                       /* CLASS                */
     0x00,                       /* Subclass             */
     0x00,                       /* Protocol             */
     0x09,                       /* bMaxPktSize0         */    // FW will update this value automatically.
     0x51,0x04,                  /* idVendor             */
     0x60,0x92,                  /* idProduct            */
     0x00,0x01,                  /* bcdDevice            */
     0x01,                       /* iManufacturer        */
     0x02,                       /* iProduct             */
     0x03,                       /* iSerial Number       */
     0x01,                       /* One configuration    */
     
     /* Device Qualifier (USB 2.0 only) */
     0x0A,                               // Length of this descriptor (10 bytes)
     USB_DT_DEVICE_QUALIFIER,           // Type code of this descriptor (06h)
     0x00,0x02,                          // Release of USB spec (Rev 2.0)
     0x00,                               // Device's base class code - vendor specific
     0x00,                               // Device's sub class code
     0x00,                               // Device's protocol type code
     0x40,                               // End point 0's packet size 64 bytes
     0x01,                               // Number of configurations supported
     0x00,                               // Reserved for future use
     
     /* Strings */
     0x04,
     USB_DT_STRING,
     0x09, 0x04                 	/* English (U.S.) */
 };
 
 uchar BOT_Descriptors [86] = 
 {      
     /* BOS */
     0x05,           	/* bLength */
     USB_DT_BOS,  
     0x2A, 0x00,     	/* wTotalLength */
     0x03,           	/* bNumDeviceCaps */
     
     /* USB 2.0 Extension Capability */
     0x07,
     USB_DT_DEVICE_CAPABILITY,
     0x02,                       	/* USB_DC_USB2_EXTENSION */
     0x02, 0x00, 0x00, 0x00,  /* LPM support */
     
     /* SuperSpeed Capability */
     0x0A,
     USB_DT_DEVICE_CAPABILITY,
     0x03,           	/* USB_DC_SUPERSPEED_USB */
     0x00,           	// LTM not supported. BQ - check this later.
     0x0E, 0x00,     /* Full, High, Super speeds supported */
     0x01,           /* Functionality support (FS) */
     0x0A,           /* U1 exit latency < 10us */
     0x0A, 0x00,     /* U2 exit latency < 10us */

     /* Container ID */
     0x14,
     USB_DT_DEVICE_CAPABILITY,
     0x04,                       /* USB_DC_CONTAINER_ID */
     0x00,
     0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
     0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,

     /* Configuration */
     0x09,                       /* bLength              */
     USB_DT_CONFIG,              /* CONFIGURATION        */
     0x26, 0x00,                 /* wTotallength         */   // FW will update this value automatically.
     TOTAL_NUM_INTERFACES,       /* bNumInterfaces       */
     0x01,                       /* bConfigurationValue  */
     0x00,                       /* iConfiguration       */
     0xE0,                       /* bmAttributes (rsvd (bit 7) + self-powered (bit 6) + remote wakeup cap (bit 5)) */  
     0x0D,                       /* bMaxPower            */   /* 0xD is 104mA */

     /* Interface (BOT) */
     0x09,                       /* bLength              */
     USB_DT_INTERFACE,           /* INTERFACE            */
     UMS_INTERFACE_NUM,          /* bInterfaceNumber     */
     0x00,                       /* bAlternateSetting    */
     0x02,                       /* bNumEndpoints        */
     0x08,                       /* bInterfaceClass      */
     0x06,                       /* bInterfaceSubClass   */
     0x50,                       /* bInterfaceProtocol   */
     0x00,                       /* iInterface           */

     /* Endpoint Descriptor  : BOT Bulk-In */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x83,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
     0x00,                       /* bInterval            */

     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,  /* bMaxBurst */
     0x00,            /* MaxStreams = 0 */
     0x00, 0x00,
     
     /* Endpoint Descriptor  : BOT Bulk-Out */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x03,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */   // FW will update this value automatically.
     0x00,                       /* bInterval            */

     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,  /* bMaxBurst */
     0x00,            /* MaxStreams = 0 */
     0x00, 0x00     
 };

 uchar /*DescriptorDeclarations::*/UAS_Descriptors [77] =
 {
     /* Interface (UAS) */
     0x09,                       /* bLength              */
     USB_DT_INTERFACE,           /* INTERFACE            */
     UMS_INTERFACE_NUM,          /* bInterfaceNumber     */
     0x01,                       /* bAlternateSetting    */
     0x04,                       /* bNumEndpoints        */
     0x08,                       /* bInterfaceClass      */
     0x06,                       /* bInterfaceSubClass   */
     0x62,                       /* bInterfaceProtocol  UAS = 0x62.  */
     0x00,                       /* iInterface           */
     
     /* Endpoint Descriptor  : UAS Bulk-In STATUS */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x82,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
     0x00,                       /* bInterval            */
     
     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,         /* bMaxBurst */
     MAX_STREAMS,            /* MaxStreams */
     0x00, 0x00,
     
     /* Pipe Usage */
     0x04,
     USB_DT_PIPE_USAGE,
     0x02,  /* Pipe ID */
     0x00,  /* RSVD */
     
     /* Endpoint Descriptor  : UAS Bulk-Out COMMAND */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x02,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically. 
     0x00,                       /* bInterval            */
     
     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,  /* bMaxBurst */
     0x00,            /* MaxStreams = 0 */
     0x00, 0x00,
     
     /* Pipe Usage */
     0x04,
     USB_DT_PIPE_USAGE,
     0x01,  /* Pipe ID */
     0x00,  /* RSVD */
     
     /* Endpoint Descriptor  : UAS Bulk-In DATA */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x83,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
     0x00,                       /* bInterval            */
     
     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,  /* bMaxBurst */
     MAX_STREAMS,     /* MaxStreams */
     0x00, 0x00,
     
     /* Pipe Usage */
     0x04,
     USB_DT_PIPE_USAGE,
     0x03,  /* Pipe ID */
     0x00,  /* RSVD */
     
     /* Endpoint Descriptor  : UAS Bulk-Out DATA */
     0x07,                       /* bLength              */
     USB_DT_ENDPOINT,            /* ENDPOINT             */
     0x03,                       /* bEndpointAddress     */
     0x02,                       /* bmAttributes         */
     0x00, 0x04,                 /* wMaxPacketSize       */  // FW will update this value automatically.
     0x00,                       /* bInterval            */
     
     /* Endpoint Companion */
     0x06,
     USB_DT_SUPERSPEED_ENDPT_COMPANION,
     MAX_BURST_SIZE,  /* bMaxBurst */
     MAX_STREAMS,     /* MaxStreams */
     0x00, 0x00,
     
     /* Pipe Usage */
     0x04,
     USB_DT_PIPE_USAGE,
     0x04,  /* Pipe ID */
     0x00  /* RSVD */ 
};

uchar HID_Descriptors [91] = 
{ 
    /*Interface (HID)*/
    0x09,                       /* bLength              */
    USB_DT_INTERFACE,           /* INTERFACE            */
    HID_INTERFACE_NUM,          /* bInterfaceNumber     */
    0x00,                       /* bAlternateSetting    */
    0x02,                       /* bNumEndpoints        */
    0x03,                       /* bInterfaceClass      */
    0x00,                       /* bInterfaceSubClass   */
    0x00,                       /* bInterfaceProtocol   */
    0x00,                       /* iInterface           */
    
    /*HID Descriptor*/
    0x09,                       /* bLength              */
    USB_DT_HID,                 /* bDescriptorType      */
    0x10, 0x01,                 /* bcdHID               */
    0x00,                       /* bCountryCode         */
    0x01,                       /* bNumDescriptor       */
    0x22,                       /* bDescriptorType      */
    0x2F, 0x00,                 /* wDescriptorLen       */
    
    /*HID Report Descriptor*/
    0x06, 0xB0, 0xFF,           /* Usage page (vendor defined)          */
    0x09, 0x01,                 /* Usage ID (vendor defined)            */
    0xA1, 0x01,                 /* Collection (application)             */
    
    /*The Input report*/
    0x09, 0x03,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x09,                 /* Report Count (5 fields)              */
    0x81, 0x02,                 /* Input (Data, Variable, Absolute)     */
    
    /*The Output report*/
    0x09, 0x04,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x09,                 /* Report Count (5 fields)              */
    0x91, 0x02,                 /* Output (Data, Variable, Absolute)    */
    
    /*The Feature report*/
    0x09, 0x05,                 /* Usage ID - vendor defined            */
    0x15, 0x00,                 /* Logical Minimum (0)                  */
    0x26, 0xFF, 0x00,           /* Logical Maximum (255)                */
    0x75, 0x08,                 /* Report Size (8 bits)                 */
    0x95, 0x02,                 /* Report Count (2 fields)              */
    0xB1, 0x02,                 /* Feature (Data, Variable, Absolute)   */
    
    0xC0,                       /* end collection */
    
    /*Endpoint Descriptor  : Burner HID Interrupt IN */
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x81,                       /* bEndpointAddress     */
    0x03,                       /* bmAttributes         */
    0x09, 0x00,                 /* wMaxPacketSize       */
    0x02,                       /* bInterval            */
    
    /*Endpoint Companion*/
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    0x00,             /* bMaxBurst */
    0x00,             /* MaxStreams = 0 */
    0x00, 0x00,
    
    /*Endpoint Descriptor  : Burner HID Interrupt OUT*/
    0x07,                       /* bLength              */
    USB_DT_ENDPOINT,            /* ENDPOINT             */
    0x01,                       /* bEndpointAddress     */
    0x03,                       /* bmAttributes         */
    0x09, 0x00,                 /* wMaxPacketSize       */
    0x02,                       /* bInterval            */
    
    /*Endpoint Companion*/
    0x06,
    USB_DT_SUPERSPEED_ENDPT_COMPANION,
    0x00,  /* bMaxBurst */
    0x00,  /* MaxStreams = 0 */
    0x00, 0x00
    	
};

typedef struct {
	uchar* Buffer;
	uint BufferSize;
}Output_Buffer;

/*QString bMaxPower_value ="0D";
*/

#endif

