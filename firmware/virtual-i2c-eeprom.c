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

#include "virtual-i2c-eeprom.h"
#include <pic18f14k50.h>
#include "usb_device_cdc.h"


/**

Virtual EEPROM emulation

Addresses
 * 0x50 - 1 byte addresses (max 256 bytes)
 * 0x51 - 2 byte addresses (max 16k bytes)

"Writing" to the EEPROM is putting a new address into the address register.

"Reading" will return the data pointed by the current address and increment the address value by one.

 */

// EEPROM ADDR
// 1010   000
#define EEPROM_ADDR 0x51
#define EEPROM_SIZE 16

static uint16_t veeprom_addr = 0;
static uint16_t veeprom_addr_shadow = 0;
static uint8_t veeprom_data[EEPROM_SIZE] = {
	0xC0, 0x19, 0x2A, 0x40, 0x54, 0x5, 0x0, 0x0, 0xA, 0xB, 0xC,
	};

//#define SSPCON1.CKP
#define I2CMODE_SLAVE_7BIT 0x06


void veeprom_reset(void)
{
    if (SSPCON1bits.WCOL) {
        SSPCON1bits.WCOL = 0;   // Clear any write collision bit
        SSPBUF;                 // Flush SSPBUF
    }
    if (SSPCON1bits.SSPOV) {
        SSPCON1bits.SSPOV = 0;  // Clear any overflow bit
    }    
}

void veeprom_init(void)
{
    veeprom_addr = 0;
    
    // Set SCL and SDA as inputs
    TRISBbits.RB6 = 1; // SCL
    TRISBbits.RB4 = 1; // SDA
    
    // Setup the SSPSTAT register
    SSPSTATbits.SMP = 0;    // Disable Slew rate control
    SSPSTATbits.CKE = 0;    // Disable SMBBus Select Bit
    //SSPSTATbits.DA
    //SSPSTATbits.P
    //SSPSTATbits.S
    //SSPSTATbits.RW
    //SSPSTATbits.UA
    //SSPSTATbits.BF
    
    // Setup the SSPCON1 register
    SSPCON1bits.WCOL = 0;   // Clear write collision bit
    SSPCON1bits.SSPOV = 0;  // Clear overflow bit
    SSPCON1bits.SSPEN = 1;  // Enable the MSSP
    SSPCON1bits.CKP = 1;    // Release clock
	// Set us into I2C Slave Mode, 7 bit address
    SSPCON1bits.SSPM = I2CMODE_SLAVE_7BIT;
    
    // Setup the SSPCON2 register
    SSPCON2bits.GCEN = 0;   // Disable general cool
    //SSPCON2bits.ACKSTAT   // Master only
    //SSPCON2bits.ACKDT     // Master only
    //SSPCON2bits.ACKEN     // Master only
    //SSPCON2bits.RCEN      // Master only
    //SSPCON2bits.PEN       // Master only
    //SSPCON2bits.RSEN      // Master only
    SSPCON2bits.SEN = 1;    // Clock stretching for receive and send
    
	// Set the address the I2C will respond to bits 7 to 1
	SSPADD = EEPROM_ADDR << 1;

    // Enable interrupt priority levels
    //RCONbits.IPEN = 1;
	// WRONG: Make the MSSP run on low priority interrupt as USB is running on the 
    // high priority one.
    //IPR1bits.SSPIP = 1;
    
    // Disable the MSSP interrupts
    PIE1bits.SSPIE = 0;

    // PEIE
    
    veeprom_reset();
}

volatile char debug_data[] = {
    //E   A/D  P   S  R/W  UA  BF  C  vea
    'E', 'F','.','.','F','.','.','0', '_', '\r','\n','\0'};
static bool debug_data_sent = false;

void veeprom_service(void) {
    if(USBUSARTIsTxTrfReady()) {
        if (debug_data_sent) {
            debug_data[0] = '\0';
            debug_data_sent = false;
        }
        if (debug_data[0] == '\0')
            veeprom_interrupt();
        if (debug_data[0] != '\0') {
            //putsUSBUSART((char*)&(debug_data_test[0]));
            putsUSBUSART((char*)&(debug_data[0]));
            debug_data_sent = true;
        }
    }
}

inline void veeprom_set_addr(uint16_t addr)
{
	while(addr > EEPROM_SIZE) {
		addr -= EEPROM_SIZE;
    }
	veeprom_addr = addr;
}

inline void veeprom_set_addr_msb(uint8_t addr) {
    veeprom_addr_shadow = ((uint16_t)addr) << 8;
}

inline void veeprom_set_addr_lsb(uint8_t addr) {
    veeprom_addr_shadow |= addr;
    veeprom_set_addr(veeprom_addr_shadow);
}

inline void veeprom_addr_inc(void) {
    veeprom_set_addr(veeprom_addr+1);
}


/*
Receiving;

An MSSP interrupt is generated for each data transfer byte. Flag bit, SSPIF of
the PIR1 register, must be cleared by software.

When the SEN bit of the SSPCON2 register is set, SCL will be held low (clock
stretch) following each data transfer. The clock must be released by setting
the CKP bit of the SSPCON1 register. See Section 15.3.4 “Clock Stretching” for
more detail.
*/
inline void veeprom_write(uint8_t data_count, uint8_t data)
{
    switch(data_count) {
        case 0:
            veeprom_set_addr_msb(data);
        case 1:
            veeprom_set_addr_lsb(data);
        default:
            return;
    }
}

/*
Sending;

The transmit data must be loaded into the SSPBUF register which also loads the
SSPSR register. Then pin SCK/SCL should be released by setting the CKP bit of
the SSPCON1 register.

An MSSP interrupt is generated for each data transfer byte. The SSPIF bit must
be cleared by software and the SSPSTAT register is used to determine the status
of the byte.
*/
uint8_t veeprom_read(uint8_t data_count)
{
	// Load the data
	uint8_t data = veeprom_data[veeprom_addr];
	veeprom_addr_inc();
    return data;
}

static int8_t i2c_data_count = -2;
static uint8_t i2c_data_value = 0xff;

void veeprom_interrupt(void)
{
    if(!PIR1bits.SSPIF) {
        return;
    }
    PIR1bits.SSPIF = 0;     // Clear interrupt
    
    debug_data[0] = 'E';
    if (!SSPSTATbits.DA) {
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
    if(SSPSTATbits.RW) {
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
        i2c_data_count = -1;
    } else {
        i2c_data_count++;
    }

    
    if (SSPSTATbits.BF == 1) {
        i2c_data_value = SSPBUF;
    }
    
    debug_data[7] = '0' + i2c_data_count;
    debug_data[8] = '0' + veeprom_addr;
    
    if (SSPSTATbits.R_NOT_W) {
        if (!SSPSTATbits.STOP) {
            i2c_data_value = veeprom_read(i2c_data_count);
        }
    } else {
        veeprom_write(i2c_data_count, i2c_data_value);
    }
    
    if (!SSPSTATbits.STOP && SSPSTATbits.BF == 0) {
        SSPBUF = i2c_data_value;
    }
    SSPCON1bits.CKP = 1;    // Stop clock stretching
}
