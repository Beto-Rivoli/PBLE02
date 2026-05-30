#ifndef EEPROM_24LC512_H
#define EEPROM_24LC512_H

#include <stdint.h>


#define EEPROM_I2C_ADDR     0x50

#define EEPROM_SIZE         65536

#define EEPROM_PAGE_SIZE    128

#define EEPROM_WRITE_CYCLE_TIME_MS  5

void I2C_EEPROM_Init(void);

void EEPROM_WriteByte(uint16_t mem_addr, uint8_t data);

void EEPROM_ReadByte(uint16_t mem_addr, uint8_t *data);

void EEPROM_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t len);

void EEPROM_ReadSequential(uint16_t mem_addr, uint8_t *data, uint16_t len);

#endif /* EEPROM_24LC512_H */
