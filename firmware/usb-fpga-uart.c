
#include <p18f14k50.h>

#include "usb-fpga-uart.h"
#include "prj_usb_config.h"
#include "cdc.h"
#include "debug.h"

void usb_fpga_uart_reset(void) {
	unsigned char c = 0;
	RCSTAbits.CREN = 0; // reset the port
	// Flush the FIFO
	c = RCREG;
	c = RCREG;
	RCSTAbits.CREN = 1; // and keep going.
}

void usb_fpga_uart_init(void) {
	unsigned char c;

	//Configure UART

	// Clear the port B and latches 
	PORTB = 0x0;
	LATB = 0x0;

	// Enable weak pull ups for when nothing connected
	WPUBbits.WPUB7 = 1; 
	WPUBbits.WPUB5 = 1;

	// No analog on RB7
	// Disable Analog on port on RB5
	ANSELHbits.ANS11 = 0;

	TRISBbits.TRISB7 = 0; // TX - Output
	TRISBbits.TRISB5 = 1; // RX - Input

	// 0b00100100
	// 7 CSRC=0 -- Master mode
	// 6 TX9 =0 -- 8 bit mode
	// 5 TXEN=1 -- Transmit enable
	// 4 SYNC=0 -- Async mode
	// 3 SENB=0 -- Sync break complete
	// 2 BRGH=1 -- High speed
	// 1 TRMT=0 -- Transmit full
	// 0 TX9D=0 -- 9th bit
	TXSTA = 0x24;
	// 0b10010000
	// 7 SPEN=1 -- Enable serial port
	// 6 RX9 =0 -- 8 bit mode
	// 5 SREN=0 -- Don't care (Async mode)
	// 4 CREN=1 -- Enables receiver
	// 3 ADDE=0 -- Address Detect disabled
	// 2 FERR=0 -- Framing Error
	// 1 OERR=0 -- Over run error
	// 0 RX9D=0 -- 9th bit
	RCSTA = 0x90;		// Single Character RX
	SPBRG = 0xE1;
	SPBRGH = 0x04;		// 0x04E1 for 48MHz -> 9600 baud rate
	BAUDCON = 0x08;		// BRG16 = 1
	usb_fpga_uart_reset();
}

void usb_fpga_uart_service(void) {
	unsigned char c = 0;

  	// in case of overrun error
      	// we should never see an overrun error, but if we do, 
	if (RCSTAbits.OERR) {
		debug_send_cdata("RX Err: Overrun\r\n");
		usb_fpga_uart_reset();
	}
	if (RCSTAbits.FERR) {
		debug_send_cdata("RX Err: Framing\r\n");
		usb_fpga_uart_reset();
	}

	if(TXSTAbits.TRMT && cdc_peek_getc(USB_FPGA_PORT, &c)) {
		debug_send_cdata("TX\r\n");
		c = cdc_getc(USB_FPGA_PORT);
		TXREG = c;
	}

	if (PIR1bits.RCIF) {
		debug_send_cdata("RX\r\n");
		c = RCREG;
		cdc_putc(USB_FPGA_PORT, c);
	}
}
