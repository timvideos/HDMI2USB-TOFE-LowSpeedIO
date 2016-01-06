
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

#define MAX_COMMAND_SIZE 32

#pragma code
uint8_t recv_buffer_i = 0;
char recv_buffer[MAX_COMMAND_SIZE];

typedef void (* PTR_CLI_FUNCTPTR)(char* args);
struct command {
	const rom char* name; //[MAX_COMMAND_SIZE];
	const PTR_CLI_FUNCTPTR handler;
	const rom char* help_short;
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
//	{"debug", command_debug, "Control debug output.",
//	},
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

#define pic_uart_putc(c) \
	cdc_putc(USB_PIC_PORT, c);
void pic_uart_send_cstr(const rom char* s) {
	cdc_put_cstr(USB_PIC_PORT, s);
}
void pic_uart_send_newline();
void pic_uart_send_newline() {
	pic_uart_send_cstr("\r\n");
}

void _pic_uart_send_hex_nibble(uint8_t i) {
	i &= i;
	if (i > 9) {
		pic_uart_putc('A'+(i-10));
	} else if (i <= 16) {
		pic_uart_putc('0'+i);
	} else {
		pic_uart_putc('?');
	}
}
void pic_uart_send_hex_byte(uint8_t i) {
	_pic_uart_send_hex_nibble(MSN(i));
	_pic_uart_send_hex_nibble(LSN(i));
}

void usb_pic_uart_init(void) {
	// Output the prompt
	recv_buffer_i = 0;
}

void usb_pic_uart_service(void) {
	uint8_t i;
	unsigned char c;

	while (cdc_peek_getc(USB_PIC_PORT, &c)) {
		veeprom_set(VEEPROM_LED_START, !veeprom_get(VEEPROM_LED_START));

		c = cdc_getc(USB_PIC_PORT);

		// Clear current command
		if (c == 0x3) {
			pic_uart_send_cstr("^C");
			recv_buffer_i = 0;
			c = '\n';
		}
		// Command complete
		if (c == '\n' || c == '\r') {
			pic_uart_send_newline();
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
					pic_uart_send_cstr("Unknown command '");
					for(i = 0; i < recv_buffer_i; i++) {
						pic_uart_putc(recv_buffer[i]);
					}
					pic_uart_send_cstr("'. Try 'help'.");
				}
				pic_uart_send_newline();
			}

			// Output the prompt
			pic_uart_putc('>');
			pic_uart_putc(' ');
			recv_buffer_i = 0;

		// Allow backspace & delete
		} else if ((c == '\b' || c == 177 || c == 127)) {
			if (recv_buffer_i > 0) {
				pic_uart_putc('\b');
				pic_uart_putc(' ');
				pic_uart_putc('\b');
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
				pic_uart_send_hex_byte(c);
				continue;
			}

			pic_uart_putc(c);		// Remote Echo
			recv_buffer[recv_buffer_i++] = c;
			if (recv_buffer_i > (MAX_COMMAND_SIZE-1)) {
				pic_uart_send_cstr("Command too long.\r\n");
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
			// Skip internal functions unless given "help all"
			if (all || pcmd->help_short != NULL) {
				pic_uart_send_cstr(pcmd->name);
				pic_uart_send_newline();
			}
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
			pic_uart_send_cstr("Unknown command.");
			return;
		}
		pic_uart_send_cstr(pcmd->name);
		pic_uart_send_cstr("\r\n-----\r\n");
		if (pcmd->help_short) {
			pic_uart_send_cstr(pcmd->help_short);
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
			pic_uart_send_cstr("Enabling channel ");
			_pic_uart_send_hex_nibble(channel);
			_adc[channel].enabled = true;
			return;
		} else if (startswith("off", args)) {
			pic_uart_send_cstr("Disabling channel ");
			_pic_uart_send_hex_nibble(channel);
			_adc[channel].enabled = false;
			return;
		} else {
			pic_uart_send_cstr("Unknown command.");
			return;
		}
	}

	for (i = 0; i < ADC_CHANNELS; i++) {
		if (channel != -1 && channel != i) {
			continue;
		}

		pic_uart_send_cstr("Channel ");
		_pic_uart_send_hex_nibble(i);
		if (_adc[i].enabled) {
			pic_uart_send_cstr(" on  ");
		} else {
			pic_uart_send_cstr(" off ");
		}
		pic_uart_send_cstr("Value: 0x");
		pic_uart_send_hex_byte(_adc[i].data.h);
		pic_uart_send_hex_byte(_adc[i].data.l);
		pic_uart_send_cstr(" (seq: 0x");
		pic_uart_send_hex_byte(_adc[i].updated);
		pic_uart_send_cstr(")\r\n");
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
		pic_uart_send_cstr("Dumping from 0x");
		a = MSB(p);
		pic_uart_send_hex_byte(a);
		a = LSB(p);
		pic_uart_send_hex_byte(a);
		pic_uart_send_cstr(" to 0x");
		a = MSB(end);
		pic_uart_send_hex_byte(a);
		a = LSB(end);
		pic_uart_send_hex_byte(a);
		pic_uart_send_newline();
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
			pic_uart_putc(':');
			pic_uart_send_hex_byte(line);
			sum += line;
			// Address
			{
				uint8_t addrl = LSB(p);
				uint8_t addrh = MSB(p);
				pic_uart_send_hex_byte(addrh);
				sum += addrh;
				pic_uart_send_hex_byte(addrl);
				sum += addrl;
			}
			// Type
			pic_uart_send_cstr("00");
			sum += 0x00;
		}
		// Byte of data
		pic_uart_send_hex_byte(*p);
		sum += (*p);
		p++;

		// Footer
		line--;
		if (line == 0) {
			// Check sum
			pic_uart_send_hex_byte(-sum);
			pic_uart_send_newline();
		}
	} while (p < end);
	pic_uart_send_cstr(":00000001FF\r\n");
}
#endif

void command_led(char* args) {
//	 "led (d5|d6) (on|off|toggle) - Turn on/off the LED.\r\n"
	if (args == NULL) {
	} else if (*args == 'd' || *args == 'D') {
		uint16_t addr = 0;
		if (*(args+1) == '5') {
			addr = VEEPROM_LED_START;
			pic_uart_send_cstr("LED D5 ");
		} else if (*(args+1) == '6') {
			addr = VEEPROM_LED_START+1;
			pic_uart_send_cstr("LED D6 ");
		}
		if (addr > 0) {
			uint8_t v = veeprom_get(addr);
			args = next_token(args);
			if (args == NULL) {
				if (v) {
					pic_uart_send_cstr("is on.");
				} else {
					pic_uart_send_cstr("is off.");
				}
			} else {
				if (startswith("on", args)) {
					v = 1;
				} else if (startswith("off", args)) {
					v = 0;
				} else if (startswith("toggle", args)) {
					v = !v;
				}
				veeprom_set(addr, v);
				if (v) {
					pic_uart_send_cstr("turned on.");
				} else {
					pic_uart_send_cstr("turned off.");
				}
			}
			return;
		}
	}
	pic_uart_send_cstr("Unknown command.");
}

void command_ping(char* args) {
//	 "Command to test the firmware is responding."
	pic_uart_send_cstr("PONG!");
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
			pic_uart_send_cstr("Invalid address.");
			return;
		}
		args = next_token(args);
		if (args != NULL) {
			data = atoi(args);
		}
		if (data < 0 || data > 0xff) {
			pic_uart_send_cstr("Invalid data.");
			return;
		}
		if (read) {
			while(data > 0) {
				pic_uart_send_hex_byte(veeprom_get(addr));
				pic_uart_putc(' ');
				addr++;
				data--;
			}
			return;
		} else {
			veeprom_set(addr, (uint8_t)(data & 0xff));
		}
		return;
	}
	pic_uart_send_cstr("Unknown command.");
}

bool _reboot_countdown(uint8_t c) {
	uint16_t i, j;
	pic_uart_putc('0'+c);
	cdc_flush_in_now(USB_PIC_PORT);
	for(j = 0; j < 0xf; j++) {
		for(i = 0; i < 0xffff; i++) {
#ifndef USB_INTERRUPTS
        	        usb_handler();
#endif
		}
		pic_uart_putc('.');
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

	// Disable watchdog
	WDTCONbits.SWDTEN = 0;

	return true;
}

void command_reboot(char* args) {
//         "reboot       - Just reboot.\r\n"
//         "reboot flash - Reboot into firmware upgrade mode."
	bool flash = (args != NULL && *args == 'f');

	pic_uart_send_cstr("Rebooting");
	if (flash) {
		pic_uart_send_cstr(" to the firmware bootloader");
	}
	pic_uart_send_newline();

	if(!_reboot_prepare()) {
		pic_uart_send_cstr("Aborting!\r\n");
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
		pic_uart_send_hex_byte(i);
		if (i % 16 == 0) {
			pic_uart_send_newline();
		}
		i++;
	}
}


void command_usb_reset(char* args) {
	usb_hard_reset();
}
#endif

void command_version(char* args) {
	pic_uart_send_cstr(GIT_VERSION_HUMAN);
	pic_uart_send_newline();
	pic_uart_send_cstr(GIT_VERSION_HASH);
	pic_uart_send_newline();
}
