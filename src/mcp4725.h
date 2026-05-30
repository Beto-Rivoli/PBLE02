#ifndef MCP4725_H
#define MCP4725_H

// Endereço I2C padrão do MCP4725 (A0 conectado ao GND)
#define MCP4725_ADDR 0x60

// Inicializa o DAC (pode estar vazia por enquanto, depende do modo de uso)
void mcp4725Init(void);

// Define a saída do DAC (0 a 4095 - 12 bits)
void mcp4725SetOutput(unsigned int value);

// Define a saída do DAC e salva na EEPROM (pode levar 25ms para gravar)
void mcp4725SetOutputEEPROM(unsigned int value);

#endif // MCP4725_H
