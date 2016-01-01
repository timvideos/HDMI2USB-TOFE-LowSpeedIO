/**

Virtual EEPROM emulation

Addresses
 * 0x50 - 1 byte addresses (max 256 bytes)
 * 0x51 - 2 byte addresses (max 16k bytes)

"Writing" to the EEPROM is putting a new address into the address register.

"Reading" will return the data pointed by the current address and increment the address value by one.

This file makes the PIC appear as an 24 series EEPROM on it's I2C interface.
 */

#include <p18f14k50.h>

#include "virtual-i2c-eeprom.h"
#include "adc.h"
#include "debug.h"
#include "descriptors.h"
#include "dp_usb/usb_stack.h"


// Virtual EEPROM get functions
uint8_t _veeprom_get_flash(uint16_t addr);
uint8_t _veeprom_get_eeprom(uint8_t addr);
uint8_t _veeprom_get_adc(uint8_t addr);
uint8_t _veeprom_get_usb(uint8_t addr);
uint8_t _veeprom_get_led(uint8_t addr);
uint8_t veeprom_get(uint16_t addr) {
	// Internal Flash Area
	if (addr >= VEEPROM_FLASH_START && addr <= VEEPROM_FLASH_END) {
		return _veeprom_get_flash(addr-VEEPROM_FLASH_START);
	}
	// EEPROM Area
	if (addr >= VEEPROM_EEPROM_START && addr <= VEEPROM_EEPROM_END) {
		return _veeprom_get_eeprom((uint8_t)(addr&0xff));
	}
	// ADC Area
	if (addr >= VEEPROM_ADC_START && addr <= VEEPROM_ADC_END) {
		return _veeprom_get_adc((uint8_t)(addr&0xff));
	}
	// USB Area
	if (addr >= VEEPROM_USB_START && addr <= VEEPROM_USB_END) {
		return _veeprom_get_usb((uint8_t)(addr&0x0f));
	}
	// LED Area
	if (addr >= VEEPROM_LED_START && addr <= VEEPROM_LED_END) {
		return _veeprom_get_led((uint8_t)(addr&0x0f));
	}
	return 0xff;
}

// Virtual EEPROM set functions
void _veeprom_set_flash(uint16_t addr, uint8_t data);
void _veeprom_set_eeprom(uint8_t addr, uint8_t data);
void _veeprom_set_adc(uint8_t addr, uint8_t data);
void _veeprom_set_usb(uint8_t addr, uint8_t data);
void _veeprom_set_led(uint8_t addr, uint8_t data);
void veeprom_set(uint16_t addr, uint8_t data) {
	// Internal Flash Area
	if (addr >= VEEPROM_FLASH_START && addr <= VEEPROM_FLASH_END) {
		_veeprom_set_flash(addr-VEEPROM_FLASH_START, data);
	}
	// EEPROM Area
	if (addr >= VEEPROM_EEPROM_START && addr <= VEEPROM_EEPROM_END) {
		_veeprom_set_eeprom((uint8_t)(addr&0xff), data);
	}
	// ADC Area
	if (addr >= VEEPROM_ADC_START && addr <= VEEPROM_ADC_END) {
		_veeprom_set_adc((uint8_t)(addr&0xff), data);
	}
	// USB Area
	if (addr >= VEEPROM_USB_START && addr <= VEEPROM_USB_END) {
		_veeprom_set_usb((uint8_t)(addr&0x0f), data);
	}
	// Led Area
	if (addr >= VEEPROM_LED_START && addr <= VEEPROM_LED_END) {
		_veeprom_set_led((uint8_t)(addr&0x0f), data);
	}
}

// Large data array stored in the PIC Flash
extern const rom uint8_t _veeprom_flash_data[];
extern const rom uint16_t _veeprom_flash_size;
uint8_t _veeprom_get_flash(uint16_t addr) {
	if (addr < _veeprom_flash_size) {
		return _veeprom_flash_data[addr];
	} else {
		return 0xfe;
	}
}
void _veeprom_set_flash(uint16_t addr, uint8_t data) {
	// Read only
}

// PIC EEPROM access
uint8_t _veeprom_get_eeprom(uint8_t addr) {
	EEADR = addr;
	EECON1 = 0;
	EECON1bits.RD = 1;
	return EEDATA;
}
void _veeprom_set_eeprom(uint8_t addr, uint8_t data) {
	uint8_t oldGIEH;
	EEADR = addr;
	EECON1 = 0;
	EEDATA = data;
	EECON1 = 0;
	EECON1bits.WREN = 1;

	// Disable Interrupts - Required Sequence
	oldGIEH = INTCONbits.GIEH;
	INTCONbits.GIE = 0;
	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
	INTCONbits.GIEH = oldGIEH;
	while(EECON1bits.WR);
	EECON1bits.WREN = 0;
}

// PIC ADC access
/*
ADC Region Map

+--------+--------+--------------+-----+-------------------------------------------------+
| Start  | End    | Size (bytes) | R/W | Usage                                           |
+--------+--------+--------------+-----+-------------------------------------------------+
| 0x06X0 | 0x06X0 | 1            | RW  | Channel X - Enable. 0 == Disabled, 1 == Enabled |
| 0x06X1 | 0x06X1 | 1            | RO  | Channel X - Update counter. Increases every time the ADC value is updated. |
| 0x06X2 | 0x06X2 | 1            | RO  | Channel X - Low byte ADC value.                 |
| 0x06X3 | 0x06X3 | 1            | RO  | Channel X - High byte ADC value.                |
+--------+--------+--------------+-----+-------------------------------------------------+

 0x06XY

 X == Channel
 Y == 0 - Enabled byte
 Y == 1 - Update counter
 Y == 2 - Low byte of ADC value
 Y == 3 - High byte of ADC value
*/

uint8_t _veeprom_get_adc(uint8_t addr) {
	uint8_t channel = (addr >> 4) & 0xf;
	uint8_t object = addr & 0xf;
	if (channel > ADC_CHANNELS)
		return 0xfd;

	switch(object) {
	case 0:
		return _adc[channel].enabled;
	case 1:
		return _adc[channel].updated;
	case 2:
		return (uint8_t)(_adc[channel].data.l);
	case 3:
		return (uint8_t)(_adc[channel].data.h);
	default:
		return 0xfe;

	}
}
void _veeprom_set_adc(uint8_t addr, uint8_t data) {
	uint8_t channel = (addr >> 4) & 0xf;
	uint8_t object = addr & 0xf;
	if (channel > ADC_CHANNELS)
		return;

	switch(object) {
	case 0:
		_adc[channel].enabled = data;
	case 1:
		_adc[channel].updated = data;
	}
}

// PIC's USB Serial Number string
uint8_t _veeprom_get_usb(uint8_t addr) {
	uint8_t offset = 2 + addr*2;
	if (addr > 16)
		return 0xff;

	return cdc_str_serial[offset];
}
void _veeprom_set_usb(uint8_t addr, uint8_t data) {
	uint8_t i;
	uint8_t offset = 2 + addr*2;
	if (addr > 16)
		return;

	cdc_str_serial[offset] = data;
	if (addr == 15) {
		// Reset USB as serial number has changed.
		usb_hard_reset();
	}
}

// PIC LED access
uint8_t _veeprom_get_led(uint8_t addr) {
	if (addr > 2)
		return 0xf1;

	switch(addr) {
	case 0:
		// D5
		return PORTCbits.RC5;
	case 1:
		// D6
		return PORTCbits.RC4;
	default:
		return 0xf2;

	}
}
void _veeprom_set_led(uint8_t addr, uint8_t data) {
	uint8_t channel = (addr >> 4) & 0xf;
	uint8_t object = addr & 0xf;
	if (channel > ADC_CHANNELS)
		return;

	switch(object) {
	case 0:
		// D5
		PORTCbits.RC5 = data;
	case 1:
		// D6
		PORTCbits.RC4 = data;
	}
}



/**
 * I2C Interface
 */

// EEPROM ADDR
// 1010   001
#define EEPROM_ADDR 0x51

#define I2CMODE_SLAVE_7BIT 0x06

uint16_t _veeprom_i2c_addr_current;
uint16_t _veeprom_i2c_addr_shadow;

void veeprom_i2c_write(uint8_t data_count, uint8_t data) {
	if (data_count == 0) {
		_veeprom_i2c_addr_shadow = ((uint16_t)data) << 8;
	} else if (data_count == 1) {
		_veeprom_i2c_addr_shadow |= data;
		_veeprom_i2c_addr_current = _veeprom_i2c_addr_shadow;
	} else if (data_count > 1) {
		veeprom_set(_veeprom_i2c_addr_current, data);
		_veeprom_i2c_addr_current++;
	}
}

uint8_t veeprom_i2c_read(uint8_t data_count) {
	// Load the data
	uint8_t data = veeprom_get(_veeprom_i2c_addr_current);
	_veeprom_i2c_addr_current++;
	return data;
}

void veeprom_i2c_reset(void) {
	if (SSPCON1bits.WCOL) {
		SSPCON1bits.WCOL = 0;	// Clear any write collision bit
		SSPBUF;			// Flush SSPBUF
	}
	if (SSPCON1bits.SSPOV) {
		SSPCON1bits.SSPOV = 0;  // Clear any overflow bit
	}
}

/*

The MSSP module has seven registers for I2C operation.

 - MSSP Control Register 1 (SSPCON1)
 - MSSP Control Register 2 (SSPCON2)
 - MSSP Status register (SSPSTAT)
 - Serial Receive/Transmit Buffer Register (SSPBUF)
 - MSSP Shift Register (SSPSR) – Not directly accessible
 - MSSP Address Register (SSPADD)
 - MSSP Address Mask (SSPMSK)


SSPSTAT
  7: SMP: Slew Rate Control bit
  6: CKE: SMBus Select bit
  5: D/A: Data/Address bit
  4: P (Stop bit):  Indicates that a Stop bit has been detected last
  3: S (Start bit): Indicates that a Start bit has been detected last
  2: R/W: 1 == Read, 0 == Write
  1: UA (Update Address Bit): 10-bit slave only
  0: BF (Buffer Full): 1 == Full, 0 == Empty

SSPCON1
  7: WCOL: Write collision detect
  6: SSPOV (Receive Overflow)
  5: SSPEN (Serial Port Enable)
  4: CKP: SCK Release Control Bit: 1 == Release clock, 0 == Clock stretch
3-0: SSPM<3:0>: Serial Port Mode Select

 1111 = I2C Slave Mode, 10 bit address with Start/Stop interrupts
 1110 = I2C Slave Mode,  7 bit address with Start/Stop interrupts
 0111 = I2C Slave Mode, 10 bit address
 0110 = I2C Slave Mode   7 bit address

SSPCON2
  7: GCEN - General Call Enable bit
  6: Master Only (ACKSTAT)
  5: Master Only (ACKDT)
  4: Master Only (ACKEN)
  3: Master Only (RCEN)
  2: Master Only (PEN)
  1: Master Only (RSEN)
  0: SEN - Stretch Enable bit


Slave Mode
See section "15.3.3 SLAVE MODE" of the datasheet.

The SCL and SDA pins must be configured as inputs.


When an address is matched, or the data transfer after an address match is
received, the hardware automatically will generate the Acknowledge (ACK) pulse
and load the SSPBUF register with the received value currently in the SSPSR
register.

 */
void veeprom_i2c_init(void) {
	_veeprom_i2c_addr_current = 0;
	_veeprom_i2c_addr_shadow = 0;

	// Set the LED ports as output
	TRISCbits.RC5 = 0; // LED D5
	TRISCbits.RC4 = 0; // LED D6

	// Set SCL and SDA as inputs
	TRISBbits.RB6 = 1;	// SCL
	TRISBbits.RB4 = 1;	// SDA

	// Enable weak pull ups
	WPUBbits.WPUB6 = 1;
	WPUBbits.WPUB4 = 1;

	// No analog on RB6
	// Disable Analog on port RB4
	ANSELHbits.ANS10 = 0;

	// Setup the SSPSTAT register
	SSPSTATbits.SMP = 0;	// Disable Slew rate control
	SSPSTATbits.CKE = 0;	// Disable SMBBus Select Bit
	//SSPSTATbits.DA
	//SSPSTATbits.P -- Stop
	//SSPSTATbits.S -- Start
	//SSPSTATbits.RW
	//SSPSTATbits.UA
	//SSPSTATbits.BF

	// Setup the SSPCON1 register
	SSPCON1bits.WCOL = 0;	// Clear write collision bit
	SSPCON1bits.SSPOV = 0;	// Clear overflow bit
	SSPCON1bits.SSPEN = 1;	// Enable the MSSP
	SSPCON1bits.CKP = 1;	// Release clock
	// Set us into I2C Slave Mode, 7 bit address
	SSPCON1bits.SSPM = I2CMODE_SLAVE_7BIT;

	// Setup the SSPCON2 register
	SSPCON2bits.GCEN = 0;	// Disable general cool
	//SSPCON2bits.ACKSTAT	// Master only
	//SSPCON2bits.ACKDT	// Master only
	//SSPCON2bits.ACKEN	// Master only
	//SSPCON2bits.RCEN	// Master only
	//SSPCON2bits.PEN	// Master only
	//SSPCON2bits.RSEN	// Master only
	SSPCON2bits.SEN = 1;	// Clock stretching for receive and send

	// Set the address the I2C will respond to bits 7 to 1
	SSPADD = EEPROM_ADDR << 1;

	// Enable interrupt priority levels
	//RCONbits.IPEN = 1;
	// Make the MSSP run on low priority interrupt as USB is running on the
	// high priority one.
	//IPR1bits.SSPIP = 0;

	// Disable the MSSP interrupts
	PIE1bits.SSPIE = 0;

	veeprom_i2c_reset();
}


static int8_t _veeprom_i2c_data_count;
static uint8_t _veeprom_i2c_data_value;

uint8_t debug_data_send;
volatile char debug_data[] = {
    //E   A/D  P   S  R/W  UA  BF  C  vea
    'E', 'F','.','.','F','.','.','0', '_', '\r','\n','\0'};
static bool debug_data_sent = false;

void veeprom_i2c_interrupt(void) {
	if(!PIR1bits.SSPIF) {
		return;
	}
	PIR1bits.SSPIF = 0; // Clear interrupt

	// Dump some debug data
	debug_data[0] = 'E';
	if (!SSPSTATbits.D_A) {
		debug_data[1] = 'A';
	} else {
		debug_data[1] = 'D';
	}
	if (SSPSTATbits.P) {
		debug_data[2] = 'P';
	} else {
		debug_data[2] = '.';
	}

	if (SSPSTATbits.S) {
		debug_data[3] = 'S';
	} else {
		debug_data[3] = '.';
	}
	if(SSPSTATbits.R_W) {
		debug_data[4] = 'R';
	} else {
		debug_data[4] = 'W';
	}
	if (SSPSTATbits.UA) {
		debug_data[5] = 'A';
	} else {
		debug_data[5] = '.';
	}

	if (SSPSTATbits.BF) {
		debug_data[6] = 'F';
	} else {
		debug_data[6] = '.';
	}

	if (!SSPSTATbits.D_NOT_A) {
		_veeprom_i2c_data_count = -1;
	} else {
		_veeprom_i2c_data_count++;
	}

	if (SSPSTATbits.BF == 1) {
		_veeprom_i2c_data_value = SSPBUF;
	}

	debug_data[7] = '0' + _veeprom_i2c_data_count;
	debug_data[8] = '0' + _veeprom_i2c_addr_current;
	//debug_printf(DEBUG_INFO, DEBUG_EEPROM, "%s", debug);

	if (SSPSTATbits.R_W) {
		if (!SSPSTATbits.P) {
			_veeprom_i2c_data_value = veeprom_i2c_read(_veeprom_i2c_data_count);
		}
	} else {
		veeprom_i2c_write(_veeprom_i2c_data_count, _veeprom_i2c_data_value);
	}

	if (!SSPSTATbits.P && SSPSTATbits.BF == 0) {
		SSPBUF = _veeprom_i2c_data_value;
	}
	SSPCON1bits.CKP = 1;	// Stop clock stretching
}

void veeprom_i2c_service(void) {
	veeprom_i2c_interrupt();
}
