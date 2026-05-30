#ifndef I2C_H
#define I2C_H

void i2cInit(void);
void i2cSend(unsigned char endereco, unsigned char *dados, unsigned char qtd);
void i2cReceive(unsigned char endereco, unsigned char *dados, unsigned char qtd);
unsigned char i2cReadReg(unsigned char addr, unsigned char reg);
void i2cWriteReg(unsigned char addr, unsigned char reg, unsigned char data);

#endif // I2C_H