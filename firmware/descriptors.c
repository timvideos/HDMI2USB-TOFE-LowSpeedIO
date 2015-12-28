
#include "descriptors.h"

// JTR v0.2a

/* String identifiers */
#define USB_iManufacturer               1u
#define USB_iProduct                    2u
#define USB_iFPGA                       3u
#define USB_iPIC                        4u
#define USB_iSerialNum                  5u
#define USB_NUM_STRINGS                 6u

ROMPTR const unsigned char cdc_device_descriptor[] = {
        0x12,                                           // bLength
        USB_DEVICE_DESCRIPTOR_TYPE,                     // bDescriptorType
        0x00,                                           // bcdUSB (low byte)
        0x02,                                           // bcdUSB (high byte)
        0xEF,                                           // bDeviceClass     - Composite Device
        0x02,                                           // bDeviceSubClass  - Common
        0x01,                                           // bDeviceProtocol  - IAD
        USB_EP0_BUFFER_SIZE,                            // bMaxPacketSize

        LOWB(USB_VID),                                  // idVendor (low byte)
        HIGHB(USB_VID),                                 // idVendor (high byte)
        LOWB(USB_PID),                                  // idProduct (low byte)
        HIGHB(USB_PID),                                 // idProduct (high byte)
        LOWB(USB_DEV),                                  // bcdDevice (low byte)
        HIGHB(USB_DEV),                                 // bcdDevice (high byte)
        USB_iManufacturer,                              // iManufacturer
        USB_iProduct,                                   // iProduct
        USB_iSerialNum,                                 // iSerialNumber (none)
        USB_NUM_CONFIGURATIONS                          // bNumConfigurations
};

#define USB_CONFIG_CDC_LEN (8+9+5+4+5+5+7+9+7+7)

#define USB_CONFIG_DESC_TOT_LENGTH (9+USB_CONFIG_CDC_LEN*2)

ROMPTR const unsigned char cdc_config_descriptor[] = {
		// Configuration descriptor
        0x09,                                           // bLength
        USB_CONFIGURATION_DESCRIPTOR_TYPE,              // bDescriptorType
        LOWB(USB_CONFIG_DESC_TOT_LENGTH),               // wTotalLength (low byte)
        HIGHB(USB_CONFIG_DESC_TOT_LENGTH),              // wTotalLength (high byte)
        USB_NUM_INTERFACES,                             // bNumInterfaces
        0x01,                                           // bConfigurationValue
        0x00,                                           // iConfiguration (0=none)
        0x80,                                           // bmAttributes (0x80 = bus powered)
        0x64,                                           // bMaxPower (in 2 mA units, 50=100 mA)

		// CDC1 - Interface Association Descriptor
		// ---------------------------------------
        0x08,						// bLength
        USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,	// bDescriptorType   - The constant Interface Association (0Bh)
        0x00,						// bFirstInterface   - Number identifying the first interface associated with the function
        0x02,						// bInterfaceCount   - The number of contiguous interfaces associated with the function
        0x02,						// bFunctionClass    - Class code    - 0x02=COM Interface
        0x02,						// bFunctionSubClass - Subclass code - 0x02=ACM
        0x01,						// bFunctionProtocol - Protocol code - 0x01=AT modem
        USB_iFPGA,					// iFunction         - FPGA UART

		// CDC1 - Interface 0 descriptor
        0x09,                                           // bLength (Interface0 descriptor starts here)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x00,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x01,                                           // bNumEndpoints (excluding EP0)
        0x02,                                           // bInterfaceClass    0x02=COM Interface
        0x02,                                           // bInterfaceSubClass 0x02=ACM
        0x01,                                           // bInterfaceProtocol 0x01=AT modem
        USB_iFPGA,                                      // iInterface (none)
		// CDC1 - Header descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x00,                                           // bDescriptorSubtype (CDC header descriptor)
        0x10,                                           // bcdCDC (low byte)
        0x01,                                           // bcdCDC (high byte)
		// CDC1 - Abstract Control Management descriptor
        0x04,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x02,                                           // bDescriptorSubtype (CDC abstract control management descriptor)
        0x02,                                           // bmCapabilities
		// CDC1 - Union descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x06,                                           // bDescriptorSubtype (CDC union descriptor)
        0x00,                                           // bControlInterface
        0x01,                                           // bSubordinateInterface0
		// CDC1 - Call management descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x01,                                           // bDescriptorSubtype (Call management descriptor)
        0x01,                                           // bmCapabilities
        0x01,                                           // bInterfaceNum
		// CDC1 - Endpoint 1 IN descriptor (INTERRUPT, Not used)
        0x07,                                           // bLength (Endpoint1 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x81,                                           // bEndpointAddress
        0x03,                                           // bmAttributes (0x03=intr)
        LOWB(CDC_NOTICE_BUFFER_SIZE),                   // wMaxPacketSize (low byte)
        HIGHB(CDC_NOTICE_BUFFER_SIZE),                  // wMaxPacketSize (high byte)
        0x40,                                           // bInterval

		// CDC1 - Interface 1 descriptor
        0x09,                                           // bLength (Interface1 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x01,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x02,                                           // bNumEndpoints
        0x0A,                                           // datainterface type
        0x00,                                           // bInterfaceSubClass
        0x00,                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=functional unit, 0xFF=vendor specific)
        USB_iFPGA,                                      // iInterface
		// CDC1 - Endpoint 2 OUT descriptor (BULK)
        0x07,                                           // bLength (Enpoint2 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x02,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00,                                           // bInterval
		// CDC1 - Endpoint 2 IN descriptor (BULK)
        0x07,                                           // bLength
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x82,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00,                                           // bInterval

		// CDC2 - Interface Association Descriptor
		// ---------------------------------------
        0x08,						// bLength
        USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,	// bDescriptorType   - The constant Interface Association (0Bh)
        0x02,						// bFirstInterface   - Number identifying the first interface associated with the function
        0x02,						// bInterfaceCount   - The number of contiguous interfaces associated with the function
        0x02,						// bFunctionClass    - Class code    - 0x02=COM Interface
        0x02,						// bFunctionSubClass - Subclass code - 0x02=ACM
        0x01,						// bFunctionProtocol - Protocol code - 0x01=AT modem
        USB_iPIC,					// iFunction         - PIC UART

		// CDC2 - Interface 2
        0x09,                                           // bLength (Interface0 descriptor starts here)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x02,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x01,                                           // bNumEndpoints (excluding EP0)
        0x02,                                           // bInterfaceClass    0x02=COM Interface
        0x02,                                           // bInterfaceSubClass 0x02=ACM
        0x01,                                           // bInterfaceProtocol 0x01=AT modem
        USB_iPIC,                                       // iInterface (none)
		// CDC2 - Header descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x00,                                           // bDescriptorSubtype (CDC header descriptor)
        0x10,                                           // bcdCDC (low byte)
        0x01,                                           // bcdCDC (high byte)
		// CDC2 - Abstract Control Management descriptor
        0x04,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x02,                                           // bDescriptorSubtype (CDC abstract control management descriptor)
        0x02,                                           // bmCapabilities
		// CDC2 - Union descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x06,                                           // bDescriptorSubtype (CDC union descriptor)
        0x02,                                           // bControlInterface
        0x03,                                           // bSubordinateInterface0
		// CDC2 - Call management descriptor
        0x05,                                           // bFunctionLength
        0x24,                                           // bDescriptorType
        0x01,                                           // bDescriptorSubtype (Call management descriptor)
        0x01,                                           // bmCapabilities
        0x03,                                           // bInterfaceNum
		// CDC2 - Endpoint 3 IN descriptor (INTERRUPT, Not used)
        0x07,                                           // bLength (Endpoint1 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x83,                                           // bEndpointAddress
        0x03,                                           // bmAttributes (0x03=intr)
        LOWB(CDC_NOTICE_BUFFER_SIZE),                   // wMaxPacketSize (low byte)
        HIGHB(CDC_NOTICE_BUFFER_SIZE),                  // wMaxPacketSize (high byte)
        0x40,                                           // bInterval

		// CDC2 - Interface 3 descriptor
        0x09,                                           // bLength (Interface1 descriptor)
        USB_INTERFACE_DESCRIPTOR_TYPE,                  // bDescriptorType
        0x03,                                           // bInterfaceNumber
        0x00,                                           // bAlternateSetting
        0x02,                                           // bNumEndpoints
        0x0A,                                           // datainterface type
        0x00,                                           // bInterfaceSubClass
        0x00,                                           // bInterfaceProtocol (0x00=no protocol, 0xFE=functional unit, 0xFF=vendor specific)
        USB_iPIC,                                       // iInterface
		// CDC2 - Endpoint 4 OUT descriptor (BULK)
        0x07,                                           // bLength (Enpoint2 descriptor)
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x04,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00,                                           // bInterval
		// CDC2 - Endpoint 4 IN descriptor (BULK)
        0x07,                                           // bLength
        USB_ENDPOINT_DESCRIPTOR_TYPE,                   // bDescriptorType
        0x84,                                           // bEndpointAddress
        0x02,                                           // bmAttributes (0x02=bulk)
        LOWB(CDC_BUFFER_SIZE),                          // wMaxPacketSize (low byte)
        HIGHB(CDC_BUFFER_SIZE),                         // wMaxPacketSize (high byte)
        0x00,                                           // bInterval
};


ROM const unsigned char cdc_str_descs[] = {
        /* Language specifier */
	4, USB_STRING_DESCRIPTOR_TYPE, LOWB(USB_LANGID_English_United_States), HIGHB(USB_LANGID_English_United_States),
        /* USB_iManufacturer - 'Numato Labs' */
	22+2, USB_STRING_DESCRIPTOR_TYPE,
	'N',0,
	'u',0,
	'm',0,
	'a',0,
	't',0,
	'o',0,
	' ',0,
	'L',0,
	'a',0,
	'b',0,
	's',0,
        /* USB_iProduct - 'Low Speed I/O - TOFE Expansion Board' */
	72+2, USB_STRING_DESCRIPTOR_TYPE,
	'L',0,
	'o',0,
	'w',0,
	' ',0,
	'S',0,
	'p',0,
	'e',0,
	'e',0,
	'd',0,
	' ',0,
	'I',0,
	'/',0,
	'O',0,
	' ',0,
	'-',0,
	' ',0,
	'T',0,
	'O',0,
	'F',0,
	'E',0,
	' ',0,
	'E',0,
	'x',0,
	'p',0,
	'a',0,
	'n',0,
	's',0,
	'i',0,
	'o',0,
	'n',0,
	' ',0,
	'B',0,
	'o',0,
	'a',0,
	'r',0,
	'd',0,
        /* First CDC port  'FPGA UART' */
	18+2, USB_STRING_DESCRIPTOR_TYPE,
	'F',0, // 0
	'P',0, // 1
	'G',0, // 2
	'A',0, // 3
	' ',0, // 4
	'U',0, // 5
	'A',0, // 6
	'R',0, // 7
	'T',0, // 8
        /* First CDC port  'PIC UART' */
	16+2, USB_STRING_DESCRIPTOR_TYPE,
	'P',0, // 0
	'I',0, // 1
	'C',0, // 2
	' ',0, // 4
	'U',0, // 5
	'A',0, // 6
	'R',0, // 7
	'T',0, // 8
};

unsigned char cdc_str_serial[] = {
        /* USB_iSerialNum  '0123456789abcdef' */
	32+2, USB_STRING_DESCRIPTOR_TYPE,
	'U',0, // 0
	'n',0, // 1
	'c',0, // 2
	'o',0, // 3
	'n',0, // 4
	'f',0, // 5
	'i',0, // 6
	'g',0, // 7
	'u',0, // 8
	'r',0, // 9
	'e',0, // a
	'd',0, // b
	' ',0, // c
	':',0, // d
	'-',0, // e
	'(',0, // f
};
