#ifndef VIRTUAL_I2C_EEPROM_H
#define	VIRTUAL_I2C_EEPROM_H

#include <types.h>

#ifdef	__cplusplus
extern "C" {
#endif

void veeprom_set(uint16_t addr, uint8_t data);
uint8_t veeprom_get(uint16_t addr);

void veeprom_i2c_init(void);
void veeprom_i2c_service(void);

#ifdef	__cplusplus
}
#endif

#endif	/* VIRTUAL_I2C_EEPROM_H */
