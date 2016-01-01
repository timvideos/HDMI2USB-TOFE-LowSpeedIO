#ifndef VIRTUAL_I2C_EEPROM_H
#define	VIRTUAL_I2C_EEPROM_H

#include <types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
The virtual EEPROM contents is mapped the following;

+--------+--------+--------------+-----+------------------------------+
| Start  | End    | Size (bytes) | R/W | Usage                        |
+--------+--------+--------------+-----+------------------------------+
| 0x0000 | 0x03ff | 1024         | RO  | TOFE ID area                 |
| 0x0400 | 0x04ff |  256         | RW  | PIC's internal EEPROM        |
| 0x0600 | 0x06ff |  256         | RO  | Autopopulated ADC values     |
| 0x0700 | 0x070f |   16         | RW  | USB Serial Number            |
| 0x0800 | 0x0802 |    2         | RW  | LED Control bytes            |
+--------+--------+--------------+-----+------------------------------+
*/

#define VEEPROM_FLASH_START	0x0000
#define VEEPROM_FLASH_END	0x03ff
#define VEEPROM_EEPROM_START	0x0400
#define VEEPROM_EEPROM_END	0x04ff
#define VEEPROM_ADC_START	0x0600
#define VEEPROM_ADC_END		0x06ff
#define VEEPROM_USB_START	0x0700
#define VEEPROM_USB_END		0x070f
#define VEEPROM_LED_START	0x0800
#define VEEPROM_LED_END		0x0802

void veeprom_set(uint16_t addr, uint8_t data);
uint8_t veeprom_get(uint16_t addr);

void veeprom_i2c_init(void);
void veeprom_i2c_service(void);

#ifdef	__cplusplus
}
#endif

#endif	/* VIRTUAL_I2C_EEPROM_H */
