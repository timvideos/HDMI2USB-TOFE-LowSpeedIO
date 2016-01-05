
#include "usb-pic-uart.h"

#include <p18f14k50.h>
#include <string.h>
#include <stdlib.h>

#include "adc.h"
#include "cdc.h"
#include "debug.h"
#include "dp_usb/usb_stack.h"
#include "prj_usb_config.h"
#include "virtual-i2c-eeprom.h"
#include "version.h"

#define MAX_COMMAND_SIZE 16

#pragma code
uint8_t recv_buffer_i = 0;
char recv_buffer[MAX_COMMAND_SIZE];

typedef void (* PTR_CLI_FUNCTPTR)(char* args);
struct command {
	const rom char* name; //[MAX_COMMAND_SIZE];
	const PTR_CLI_FUNCTPTR handler;
	const rom char* help_short;
	const rom char* help_long;
};

void command_help(char* args);

void command_adc(char* args);
void command_debug(char* args);
void command_dump(char* args);
void command_led(char* args);
void command_ping(char* args);
void command_prom(char* args);
void command_reboot(char* args);
void command_usb_reset(char* args);
void command_usb_spam(char* args);
void command_version(char* args);

const rom struct command commands[] = {
	{"help", command_help, "Get help on commands.",
	},
	{"adc", command_adc, "ADC control command.",
	},
	{"debug", command_adc, "Control debug output.",
	},
#ifdef DEBUG_FLASH
	{"dump", command_dump, "Dump the PIC flash contents in Intel HEX format.",
        },
#endif
	{"led", command_led, "Toggle the LEDs.",
        },
	{"ping", command_ping, "Pong!",
	},
	{"prom", command_prom, "Virtual EEPROM probing.",
	},
	{"reboot", command_reboot, "Reboot the PIC.",
        },
#ifdef DEBUG_USB
	// Commands for debugging the USB stack.
	{"usb_reset", command_usb_reset, NULL,
	},
	{"usb_spam", command_usb_spam, NULL,
	},
#endif
	{"version", command_version, "Print firmware version.",
	},
	{ NULL, NULL, NULL },
};

char* next_token(char* s) {
	while(*s != '\0' && *s != ' ')
		s++;
	if ((*s) == '\0')
		return NULL;
	s++;
	return s;
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

void _cdc_put_hex_nibble(uint8_t i) {
	i &= i;
	if (i > 9) {
		cdc_putc(USB_PIC_PORT, 'A'+(i-10));
	} else if (i <= 16) {
		cdc_putc(USB_PIC_PORT, '0'+i);
	} else {
		cdc_putc(USB_PIC_PORT, '?');
	}
}
void cdc_put_hex_byte(uint8_t i) {
	_cdc_put_hex_nibble(MSN(i));
	_cdc_put_hex_nibble(LSN(i));
}


void usb_pic_uart_init(void) {
	// Output the prompt
	recv_buffer_i = 0;
}

void usb_pic_uart_service(void) {
	uint8_t i;
	unsigned char c;

	while (cdc_peek_getc(USB_PIC_PORT, &c)) {
		c = cdc_getc(USB_PIC_PORT);

		// Clear current command
		if (c == 0x3) {
			cdc_put_cstr(USB_PIC_PORT, "^C");
			recv_buffer_i = 0;
			c = '\n';
		}
		// Command complete
		if (c == '\n' || c == '\r') {
			cdc_putc(USB_PIC_PORT, '\r');
			cdc_putc(USB_PIC_PORT, '\n');
			recv_buffer[recv_buffer_i] = '\0';

			// Run the command
			if(recv_buffer_i > 0) {
				c = false;
				for(i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
					if (startswith(commands[i].name, recv_buffer)) {
						char* args = next_token(recv_buffer);
						commands[i].handler(args);
						c = true;
					}
				}
				if (!c) {
					cdc_put_cstr(USB_PIC_PORT, "Unknown command '");
					for(i = 0; i < recv_buffer_i; i++) {
						cdc_putc(USB_PIC_PORT, recv_buffer[i]);
					}
					cdc_put_cstr(USB_PIC_PORT, "'. Try 'help'.");
				}
				cdc_putc(USB_PIC_PORT, '\r');
				cdc_putc(USB_PIC_PORT, '\n');
			}

			// Output the prompt
			cdc_putc(USB_PIC_PORT, '>');
			cdc_putc(USB_PIC_PORT, ' ');
			recv_buffer_i = 0;

		// Allow backspace & delete
		} else if ((c == '\b' || c == 177 || c == 127)) {
			if (recv_buffer_i > 0) {
				cdc_putc(USB_PIC_PORT, '\b');
				cdc_putc(USB_PIC_PORT, ' ');
				cdc_putc(USB_PIC_PORT, '\b');
				recv_buffer_i--;
			}
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
			if (c == '_')
				i = true;
			if (!i) {
				cdc_put_hex_byte(c);
				continue;
			}

			cdc_putc(USB_PIC_PORT, c);		// Remote Echo
			recv_buffer[recv_buffer_i++] = c;
			if (recv_buffer_i > (MAX_COMMAND_SIZE-1)) {
				cdc_put_cstr(USB_PIC_PORT, "Command too long.\r\n");
				recv_buffer_i = 0;
			}
		}
	}
}

void command_help(char* args) {
//	 "help           - List available commands.\r\n"
//	 "help [command] - Provide detailed help on a command."
	const rom struct command *pcmd = &(commands[0]);

	bool all = false;
	if (args != NULL && startswith("all", args)) {
		all = true;
	}

	if (args == NULL || all) {
		// Output the list of available commands
		while(pcmd->handler != NULL) {
			int i = 0;
			// Skip internal functions unless given "help all"
			if (!all && pcmd->help_short == NULL) {
				pcmd++;
				continue;
			}

			// Output the name aligned
			while(pcmd->name[i] != '\0') {
				cdc_putc(USB_PIC_PORT, pcmd->name[i]);
				i++;
			}
			for(; i < 10; i++) {
				cdc_putc(USB_PIC_PORT, ' ');
			}
			// Output the short help for the command
			cdc_putc(USB_PIC_PORT, '-');
			cdc_putc(USB_PIC_PORT, ' ');
			if (pcmd->help_short) {
				cdc_put_cstr(USB_PIC_PORT, pcmd->help_short);
			} else {
				cdc_put_cstr(USB_PIC_PORT, "Internal Function");
			}
			cdc_putc(USB_PIC_PORT, '\r');
			cdc_putc(USB_PIC_PORT, '\n');

			pcmd++;
		}
	} else {
		// Output the detail help for an individual command.
		while(pcmd->handler != NULL) {
			if (startswith(pcmd->name, args)) {
				break;
			}
			pcmd++;
		}
		if (pcmd->handler == NULL) {
			cdc_put_cstr(USB_PIC_PORT, "Unknown command.");
			return;
		}
		cdc_put_cstr(USB_PIC_PORT, pcmd->name);
		cdc_put_cstr(USB_PIC_PORT, "\r\n-----\r\n");
		if (pcmd->help_short) {
			cdc_put_cstr(USB_PIC_PORT, pcmd->help_short);
			cdc_put_cstr(USB_PIC_PORT, "\r\n\r\n");
		}
		if (pcmd->help_long) {
			cdc_put_cstr(USB_PIC_PORT, pcmd->help_long);
		}
	}
}

void command_adc(char* args) {
//	 "adc               - Get status of all ADC channels.\r\n"
//	 "adc [channel]     - Get status of an ADC channel.\r\n"
//	 "adc [channel] on  - Enable channel capture.\r\n"
//	 "adc [channel] off - Disable channel capture.\r\n"
	uint8_t i;
	int16_t channel = -1;
	if (args != NULL) {
		channel = atoi(args);
		if (channel < 0 || channel >= ADC_CHANNELS) {
			channel = -1;
		}

		args = next_token(args);
		if (args == NULL) {
		} else if (startswith("on", args)) {
			cdc_put_cstr(USB_PIC_PORT, "Enabling channel ");
			_cdc_put_hex_nibble(channel);
			_adc[channel].enabled = true;
			return;
		} else if (startswith("off", args)) {
			cdc_put_cstr(USB_PIC_PORT, "Disabling channel ");
			_cdc_put_hex_nibble(channel);
			_adc[channel].enabled = false;
			return;
		} else {
			cdc_put_cstr(USB_PIC_PORT, "Unknown command.");
			return;
		}
	}

	for (i = 0; i < ADC_CHANNELS; i++) {
		if (channel != -1 && channel != i) {
			continue;
		}

		cdc_put_cstr(USB_PIC_PORT, "Channel ");
		_cdc_put_hex_nibble(i);
		if (_adc[i].enabled) {
			cdc_put_cstr(USB_PIC_PORT, " on  ");
		} else {
			cdc_put_cstr(USB_PIC_PORT, " off ");
		}
		cdc_put_cstr(USB_PIC_PORT, "Value: 0x");
		cdc_put_hex_byte(_adc[i].data.h);
		cdc_put_hex_byte(_adc[i].data.l);
		cdc_put_cstr(USB_PIC_PORT, " (seq: 0x");
		cdc_put_hex_byte(_adc[i].updated);
		cdc_put_cstr(USB_PIC_PORT, ")\r\n");
	}
}

void command_debug(char* args) {
//	 "debug dump                    - Dump everything in the debug ring buffer.\r\n"
//	 "debug level (error|warn|info) - Disable debug messages in [category].\r\n"
//	 "debug [category] (on|off)     - Turn on/off debug messages in [category].\r\n"
//	 "\r\n"
//	 "Valid categories are;\r\n"
//	 " * adc    - Messages related to the ADC capture.\r\n"
//	 " * uart   - Messages related to the FPGA USB UART.\r\n"
//	 " * eeprom - Messages related to the virtual EEPROM.\r\n"
}

#ifdef DEBUG_FLASH
void command_dump(char* args) {
//	 "dump [length]         - Dump flash from 0x0000 till length.\r\n"
//	 "dump [start] [length] - Dump flash from [start] to start+length.\r\n"
	const rom char* p = 0;
	const rom char* end = (const rom char*)(0x1000);
	int16_t left;
	int8_t line = 0;
	int8_t sum = 0;

	if (args) {
		end = (const rom char*)(atoi(args));
		args = next_token(args);
	}
	if (args) {
		p = end;
		end = (const rom char*)(atoi(args));
	}
	{
		uint8_t a;
		cdc_put_cstr(USB_PIC_PORT, "Dumping from 0x");
		a = MSB(p);
		cdc_put_hex_byte(a);
		a = LSB(p);
		cdc_put_hex_byte(a);
		cdc_put_cstr(USB_PIC_PORT, " to 0x");
		a = MSB(end);
		cdc_put_hex_byte(a);
		a = LSB(end);
		cdc_put_hex_byte(a);
		cdc_put_cstr(USB_PIC_PORT, "\r\n");
	}

	do {
		// Header
		if (line == 0) {
			sum = 0;
			// Length
			left = (end - p);
			if (left > 0x10) {
				line = 0x10;
			} else {
				line = (uint8_t)(left & 0x00ff);
			}
			cdc_putc(USB_PIC_PORT, ':');
			cdc_put_hex_byte(line);
			sum += line;
			// Address
			{
				uint8_t addrl = LSB(p);
				uint8_t addrh = MSB(p);
				cdc_put_hex_byte(addrh);
				sum += addrh;
				cdc_put_hex_byte(addrl);
				sum += addrl;
			}
			// Type
			cdc_put_cstr(USB_PIC_PORT, "00");
			sum += 0x00;
		}
		// Byte of data
		cdc_put_hex_byte(*p);
		sum += (*p);
		p++;

		// Footer
		line--;
		if (line == 0) {
			// Check sum
			cdc_put_hex_byte(-sum);
			cdc_put_cstr(USB_PIC_PORT, "\r\n");
		}
	} while (p < end);
	cdc_put_cstr(USB_PIC_PORT, ":00000001FF\r\n");
}
#endif

void command_led(char* args) {
//	 "led (d5|d6) (on|off|toggle) - Turn on/off the LED.\r\n"
	if (args == NULL) {
	} else if (*args == 'd' || *args == 'D') {
		uint16_t addr = 0;
		if (*(args+1) == '5') {
			addr = VEEPROM_LED_START;
			cdc_put_cstr(USB_PIC_PORT, "LED D5 ");
		} else if (*(args+1) == '6') {
			addr = VEEPROM_LED_START+1;
			cdc_put_cstr(USB_PIC_PORT, "LED D6 ");
		}
		if (addr > 0) {
			args = next_token(args);
			if (args == NULL) {
				if (veeprom_get(addr)) {
					cdc_put_cstr(USB_PIC_PORT, "is on.");
				} else {
					cdc_put_cstr(USB_PIC_PORT, "is off.");
				}
			} else if (startswith("on", args)) {
				veeprom_set(addr, 1);
				cdc_put_cstr(USB_PIC_PORT, "turned on.");
			} else if (startswith("off", args)) {
				veeprom_set(addr, 0);
				cdc_put_cstr(USB_PIC_PORT, "turned off.");
			} else if (startswith("toggle", args)) {
				if (veeprom_get(addr)) {
					veeprom_set(addr, 0);
					cdc_put_cstr(USB_PIC_PORT, "turned off.");
				} else {
					veeprom_set(addr, 1);
					cdc_put_cstr(USB_PIC_PORT, "turned on.");
				}
			}
			return;
		}
	}
	cdc_put_cstr(USB_PIC_PORT, "Unknown command.");
}

void command_ping(char* args) {
//	 "Command to test the firmware is responding."
	cdc_put_cstr(USB_PIC_PORT, "PONG!");
}

void command_prom(char* args) {
//	 "prom r[ead] address [len]  - Read [len] contents of the EEPROM starting at address.\r\n"
//	 "prom w[rite] address value - Write the value to EEPROM at address.\r\n"
	if (args == NULL) {
	} else if (*args == 'r' || *args == 'w') {
		bool read = (*args == 'r');
		int16_t addr = -1;
		int16_t data = (*args == 'r') ? 1 : -1;
		args = next_token(args);
		if (args != NULL) {
			addr = atoi(args);
		}
		if (addr < 0) {
			cdc_put_cstr(USB_PIC_PORT, "Invalid address.");
			return;
		}
		args = next_token(args);
		if (args != NULL) {
			data = atoi(args);
		}
		if (data < 0 || data > 0xff) {
			cdc_put_cstr(USB_PIC_PORT, "Invalid data.");
			return;
		}
		if (read) {
			while(data > 0) {
				cdc_put_hex_byte(veeprom_get(addr));
				cdc_putc(USB_PIC_PORT, ' ');
				addr++;
				data--;
			}
			return;
		} else {
			veeprom_set(addr, (uint8_t)(data & 0xff));
		}
		return;
	}
	cdc_put_cstr(USB_PIC_PORT, "Unknown command.");
}

bool _reboot_countdown(uint8_t c) {
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
		if(cdc_peek_getc(USB_PIC_PORT, &c)) {
			return false;
		}
	}
	return true;
}

bool _reboot_prepare(void) {
	uint8_t i;
	if(!_reboot_countdown(3)) {
		return false;
	}
	if(!_reboot_countdown(2)) {
		return false;
	}
	if(!_reboot_countdown(1)) {
		return false;
	}
	// Disable interrupts
	INTCON = 0x00;

	// Disable USB
	UCONbits.USBEN = 0;
	for(i = 0; i < 0xff; i++) {
		_asm nop _endasm
	}
	return true;
}

void command_reboot(char* args) {
//         "reboot       - Just reboot.\r\n"
//         "reboot flash - Reboot into firmware upgrade mode."
	bool flash = (args != NULL && *args == 'f');

	cdc_put_cstr(USB_PIC_PORT, "Rebooting");
	if (flash) {
		cdc_put_cstr(USB_PIC_PORT, "to the firmware bootloader");
	}
	cdc_put_cstr(USB_PIC_PORT, "\r\n");

	if(!_reboot_prepare()) {
		cdc_put_cstr(USB_PIC_PORT, "Aborting!\r\n");
		return;
	}

	if (flash) {
		// Initialize from c018.c
		_asm
		lfsr 1, _stack
		lfsr 2, _stack
		clrf TBLPTRU, 0
		bcf __FPFLAGS,6,0
		call 0x00e96, 0
		call 0x00fa6, 0
		goto 0x00f44
		_endasm
	} else {
		Reset();
	}
	// Should never get here...
}

#ifdef DEBUG_USB
void command_usb_spam(char* args) {
	unsigned char c = 0;
	uint8_t i = 0;
	while (!cdc_peek_getc(USB_PIC_PORT, &c)) {
		cdc_put_hex_byte(i);
		if (i % 16 == 0) {
			cdc_put_cstr(USB_PIC_PORT, "\r\n");
		}
		i++;
	}
}


void command_usb_reset(char* args) {
	usb_hard_reset();
}
#endif

void command_version(char* args) {
	cdc_put_cstr(USB_PIC_PORT, GIT_VERSION_HUMAN);
	cdc_put_cstr(USB_PIC_PORT, "\r\n");
	cdc_put_cstr(USB_PIC_PORT, GIT_VERSION_HASH);
	cdc_put_cstr(USB_PIC_PORT, "\r\n");
}
