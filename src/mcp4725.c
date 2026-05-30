#include "mcp4725.h"
#include "i2c.h"

// Inicialização (opcional para compatibilidade com o projeto)
void mcp4725Init(void) {
    // Nada necessário por padrão
}

// Envia valor ao DAC (sem salvar na EEPROM)
void mcp4725SetOutput(unsigned int value) {
    if (value > 4095) value = 4095;

    unsigned char dados[3];
    dados[0] = 0x40;                    // Comando para escrever no DAC (sem EEPROM)
    dados[1] = (value >> 4) & 0xFF;     // 8 MSBs (bits 11–4)
    dados[2] = (value & 0x0F) << 4;     // 4 LSBs (bits 3–0) alinhados à esquerda

    i2cSend(MCP4725_ADDR, dados, 3);
}


// Envia valor ao DAC e salva na EEPROM
void mcp4725SetOutputEEPROM(unsigned int value) {
    if (value > 4095) value = 4095;

    unsigned char dados[3];
    dados[0] = 0x60;                             // Comando: write DAC + EEPROM
    dados[1] = (value >> 4) & 0xFF;              // 8 MSBs
    dados[2] = (value & 0x0F) << 4;              // 4 LSBs alinhados à esquerda

    i2cSend(MCP4725_ADDR, dados, 3);
}
