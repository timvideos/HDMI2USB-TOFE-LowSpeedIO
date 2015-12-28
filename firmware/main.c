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

// PIC18F Move reset vectors for bootloader compatibility
#define REMAPPED_RESET_VECTOR_ADDRESS		0x1000
#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018

void InterruptHandlerHigh();
void InterruptHandlerLow();
void USBSuspend(void);

#pragma udata
extern BYTE usb_device_state;

void usb_fpga_service(void) {
	BYTE RecvdByte;

	// Receive and send method 1
	// The CDC module will call usb_handler each time a BULK CDC packet is sent or received.
	// If there is a byte ready will return with the number of bytes available and received byte in RecvdByte
	if (cdc_poll_getc(USB_FPGA_PORT, &RecvdByte))
		cdc_putc(USB_FPGA_PORT, RecvdByte+1);

	// Receive and send method 2
	// Same as poll_getc_cdc except that byte is NOT removed from queue.
	// This function will wait for a byte and return and remove it from the queue when it arrives.
	if (cdc_peek_getc(USB_FPGA_PORT, &RecvdByte)) { 
		RecvdByte = cdc_getc(USB_FPGA_PORT); 
		cdc_putc(USB_FPGA_PORT, RecvdByte+1);
	}

	// Receive and send method 3
	// If there is a byte ready will return with the number of bytes available and received byte in RecvdByte
	// use CDC_Flush_In_Now(); when it has to be sent immediately and not wait for a timeout condition.
	if (cdc_poll_getc(USB_FPGA_PORT, &RecvdByte)) { 
		cdc_putc(USB_FPGA_PORT, RecvdByte+1); //
		cdc_flush_in_now(USB_FPGA_PORT); 
	}
}

void usb_pic_service(void) {
	BYTE RecvdByte;

	if (cdc_peek_getc(USB_PIC_PORT, &RecvdByte)) { 
		RecvdByte = cdc_getc(USB_PIC_PORT);
		cdc_putc(USB_PIC_PORT, RecvdByte);
	}
}

#pragma code

bool cdc_flush_registered;

void main(void) {
	adc_init();
	veeprom_i2c_init();

	cdc_init(); // setup the CDC state machine
	cdc_flush_registered = false;
	usb_init(cdc_device_descriptor, cdc_config_descriptor, cdc_str_descs, USB_NUM_STRINGS);
	usb_start(); //start the USB peripheral

#if defined USB_INTERRUPTS
	EnableUsbPerifInterrupts(USB_TRN + USB_SOF + USB_UERR + USB_URST);

	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;

	EnableUsbGlobalInterrupt();
#endif

	do {
#ifndef USB_INTERRUPTS
		usb_handler();
#endif
		adc_service();
		veeprom_i2c_service();

		if (usb_device_state < CONFIGURED_STATE)
			continue;

		if (!cdc_flush_registered) {
			usb_register_sof_handler(cdc_flush_on_timeout);
			cdc_flush_registered = true;
		}
		usb_fpga_service();
		usb_pic_service();
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
extern void _startup(void);
#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS

void _reset(void) {
	_asm goto _startup _endasm
}
//set the initial vectors so this works without the bootloader too.
#pragma code HIGH_INTERRUPT_VECTOR = 0x08

void High_ISR(void) {
	_asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}
#pragma code LOW_INTERRUPT_VECTOR = 0x18

void Low_ISR(void) {
	_asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}
#endif // defined(USB_INTERRUPTS)

#pragma code
