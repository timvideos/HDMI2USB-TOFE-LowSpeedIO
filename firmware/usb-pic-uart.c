
#include "usb-pic-uart.h"

#include <string.h>

#include "prj_usb_config.h"
#include "cdc.h"
#include "debug.h"
#include "dp_usb/usb_stack.h"

#define MAX_COMMAND_SIZE 16

uint8_t recv_buffer_i = 0;
char recv_buffer[MAX_COMMAND_SIZE];

typedef void (* PTR_CLI_FUNCTPTR)(char* cmd);

struct command {
	const rom char cmd[MAX_COMMAND_SIZE];
	const PTR_CLI_FUNCTPTR handler;
};

void command_help(char* cmd);
void command_ping(char* cmd);
void command_adc(char* cmd);
void command_eeprom(char* cmd);
void command_reset(char* cmd);
void command_debug(char* cmd);
void command_flash(char* cmd);

const rom struct command commands[] = {
	{"help", command_help},
	{"ping", command_ping},
	{"adc", command_adc},
	{"prom", command_eeprom},
	{"reset", command_reset},
	{"flash", command_flash},
};

void usb_pic_uart_init(void) {
}

bool startswith(const rom char* a, const char* b) {
	while(*a != '\0') {
		if (*b == '\0')
			return false;
		if (*a != *b)
			return false;
		a++;
		b++;
	}
	return true;
}

void usb_pic_uart_service(void) {
	uint8_t i;
	unsigned char c;

	while (cdc_peek_getc(USB_PIC_PORT, &c)) {
		c = cdc_getc(USB_PIC_PORT);

		// Command complete
		if (c == '\n' || c == '\r') {
			cdc_putc(USB_PIC_PORT, '\r');
			cdc_putc(USB_PIC_PORT, '\n');
			recv_buffer[recv_buffer_i] = '\0';

			// Run the command
			if(recv_buffer_i > 0) {
				c = false;
				for(i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
					if (startswith(commands[i].cmd, recv_buffer)) {
						commands[i].handler(recv_buffer);
						c = true;
					}
				}
				if (!c) {
					cdc_put_cstr(USB_PIC_PORT, "Unknown command. Try 'help'.\r\n");
				} else {
					cdc_putc(USB_PIC_PORT, '\r');
					cdc_putc(USB_PIC_PORT, '\n');
				}
			}

			// Output the prompt
			cdc_putc(USB_PIC_PORT, '>');
			cdc_putc(USB_PIC_PORT, ' ');
			recv_buffer_i = 0;

		// Allow backspace & delete
		} else if ((c == '\b' || c == 177) && recv_buffer_i > 0) {
			cdc_putc(USB_PIC_PORT, '\b');
			cdc_putc(USB_PIC_PORT, ' ');
			cdc_putc(USB_PIC_PORT, '\b');
			recv_buffer_i--;

		// Buffer the command line
		} else {
			i = false;
			if (c >= 'a' && c <= 'z')
				i = true;
			if (c >= 'A' && c <= 'Z')
				i = true;
			if (c >= '0' && c <= '9')
				i = true;
			if (c == ' ')
				i = true;
			if (!i)
				continue;

			cdc_putc(USB_PIC_PORT, c);		// Remote Echo
			recv_buffer[recv_buffer_i++] = c;
			if (recv_buffer_i > (MAX_COMMAND_SIZE-1)) {
				cdc_put_cstr(USB_PIC_PORT, "Command too long.\r\n");
				recv_buffer_i = 0;
			}
		}
	}
}

void command_help(char* cmd) {
}

void command_ping(char* cmd) {
	cdc_put_cstr(USB_PIC_PORT, "PONG!");
}

void command_adc(char* cmd) {
}

void command_eeprom(char* cmd) {
}

void command_reset(char* cmd) {
	usb_hard_reset();
}

void _command_flash_countdown(uint8_t c) {
	uint16_t i, j;
	cdc_putc(USB_PIC_PORT, '0'+c);
	cdc_flush_in_now(USB_PIC_PORT);
	for(j = 0; j < 0xf; j++) {
		for(i = 0; i < 0xffff; i++) {
#ifndef USB_INTERRUPTS
        	        usb_handler();
#endif
		}
		cdc_putc(USB_PIC_PORT, '.');
		cdc_flush_in_now(USB_PIC_PORT);
	}
}

/* Jump to the USB boot loader */
void command_flash(char* cmd) {
	cdc_put_cstr(USB_PIC_PORT, "Rebooting the PIC\r\n");
	cdc_put_cstr(USB_PIC_PORT, "Hold flash button now...\r\n");
	_command_flash_countdown(3);
	_command_flash_countdown(2);
	_command_flash_countdown(1);
	Reset();
	cdc_put_cstr(USB_PIC_PORT, "WTF? Jump failed...\r\n");
}
