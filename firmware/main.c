// Open source PIC USB stack echo demo
// USB stack by JTR and Honken
// CC-BY
//
// USB driver files should be in '..\dp_usb\'
// Enter a USB VID and PID in prj_usb_config.h

//USB stack
#include "../dp_usb/usb_stack_globals.h"    // USB stack only defines Not function related.
#include "descriptors.h"	// JTR Only included in main.c
#include "configwords.h"	// JTR only included in main.c

#include "adc.h"
#include "virtual-i2c-eeprom.h"
#include "usb-fpga-uart.h"
#include "usb-pic-uart.h"

// PIC18F Move reset vectors for bootloader compatibility
#define REMAPPED_RESET_VECTOR_ADDRESS		0x1000
#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018

void InterruptHandlerHigh();
void InterruptHandlerLow();
void USBSuspend(void);

#pragma udata
extern BYTE usb_device_state;

#pragma code
bool usb_service_init_after_configured;

void main(void) {
	adc_init();
	veeprom_i2c_init();

	cdc_init(); // setup the CDC state machine
	usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs, cdc_str_serial, USB_NUM_STRINGS);
	usb_start(); //start the USB peripheral

	usb_service_init_after_configured = false;
	do {

		adc_service();
		veeprom_i2c_service();

#ifndef USB_INTERRUPTS
		usb_handler();
#endif
		if (usb_device_state < CONFIGURED_STATE) {
			veeprom_set(VEEPROM_LED_START, 1);
			usb_service_init_after_configured = false;
		} else if (!usb_service_init_after_configured) {
			veeprom_set(VEEPROM_LED_START, 0);
			usb_fpga_uart_init();
			usb_pic_uart_init();
			usb_service_init_after_configured = true;
		} else {
			usb_fpga_uart_service();
			usb_pic_uart_service();
		}
	} while(1);
}


// USB suspend not yet enabled
void USBSuspend(void) {}

//interrupt routines for PIC 18 and PIC24
#if defined(USB_INTERRUPTS)

// Interrupt remap chain
//
// This function directs the interrupt to the proper function depending on the
// mode set in the mode variable.
//
// USB stack on low priority interrupts,
#pragma interruptlow InterruptHandlerLow nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")
void InterruptHandlerLow(void) {
	usb_handler();
	ClearGlobalUsbInterruptFlag();
}

#pragma interrupt InterruptHandlerHigh nosave= PROD, PCLATH, PCLATU, TBLPTR, TBLPTRU, TABLAT, section (".tmpdata"), section("MATH_DATA")
void InterruptHandlerHigh(void) { //Also legacy mode interrupt.
	usb_handler();
	ClearGlobalUsbInterruptFlag();
}

// These statements remap the vector to our function
// When the interrupt fires the PIC checks here for directions
#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
void Remapped_High_ISR(void) {
	_asm goto InterruptHandlerHigh _endasm
}

#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
void Remapped_Low_ISR(void) {
	_asm goto InterruptHandlerLow _endasm
}

//relocate the reset vector
//extern void _startup(void);
//#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
//void Remapped_reset(void) {
//	_asm goto _startup _endasm
//}

//set the initial vectors so this works without the bootloader too.
#pragma code HIGH_INTERRUPT_VECTOR = 0x08
void High_ISR(void) {
	_asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}

#pragma code LOW_INTERRUPT_VECTOR = 0x18
void Low_ISR(void) {
	_asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}

//#pragma code RESET_VECTOR = 0x0
//void _reset(void) {
//	_asm goto 0xf7c _endasm
//}

#endif // defined(USB_INTERRUPTS)

#pragma code
