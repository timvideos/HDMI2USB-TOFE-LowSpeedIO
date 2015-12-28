
#include <p18f14k50.h>

#include "usb-fpga-uart.h"
#include "prj_usb_config.h"
#include "cdc.h"

void usb_fpga_uart_init(void) {
	unsigned char c;

	//Configure UART
	TRISBbits.TRISB7 = 0;
	TRISBbits.TRISB5 = 1;

	TXSTA = 0x24;		// TX enable BRGH=1
	RCSTA = 0x90;		// Single Character RX
	SPBRG = 0xE1;
	SPBRGH = 0x04;		// 0x04E1 for 48MHz -> 9600 baud rate
	BAUDCON = 0x08;		// BRG16 = 1
	c = RCREG;		// Flush the RX register
}

void usb_fpga_uart_service(void) {
	unsigned char c = 0;
  	// in case of overrun error
      	// we should never see an overrun error, but if we do, 
	if (RCSTAbits.OERR) {
		RCSTAbits.CREN = 0; // reset the port
		c = RCREG;
		RCSTAbits.CREN = 1; // and keep going.
	}

	if(TXSTAbits.TRMT && cdc_peek_getc(USB_FPGA_PORT, &c)) {
		TXREG = cdc_getc(USB_FPGA_PORT);
	}

	if (PIR1bits.RCIF) {
		cdc_putc(USB_FPGA_PORT, RCREG);
	}
}
