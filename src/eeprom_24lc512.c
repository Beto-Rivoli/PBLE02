/*******************************************************************************
 * @file    eeprom_24lc512.c
 * @brief   Driver para memória EEPROM 24LC512 (64KB) usando o driver I2C comum
 * @details Esta versão simplificada utiliza as funções de I2C providas por i2c.h,
 *          garantindo alta compatibilidade e estabilidade no barramento.
 ******************************************************************************/

#include "eeprom_24lc512.h"
#include "i2c.h"
#include "bits.h"

/**
 * @brief  Inicializa a interface de hardware I2C chamando a inicialização comum.
 */
void I2C_EEPROM_Init(void)
{
    i2cInit();
}

/**
 * @brief  Escreve um byte em um endereço específico da EEPROM.
 *         Como a EEPROM possui ciclo de escrita interna de no máximo 5ms,
 *         inserimos um atraso após a escrita.
 */
void EEPROM_WriteByte(uint16_t mem_addr, uint8_t data)
{
    unsigned char buffer[3];
    
    // Formata o endereço de memória de 16 bits (MSB primeiro) + dado
    buffer[0] = (uint8_t)(mem_addr >> 8);   // Address MSB
    buffer[1] = (uint8_t)(mem_addr & 0xFF); // Address LSB
    buffer[2] = data;

    // Envia endereço + dado para a EEPROM via I2C comum
    i2cSend(EEPROM_I2C_ADDR, buffer, 3);

    // Aguarda o ciclo de escrita interna (máximo 5 ms no datasheet).
    // A 48 MHz, um loop de 30.000 iterações com volatile garante pelo menos 5-6ms.
    for (volatile uint32_t i = 0; i < 30000; i++);
}

/**
 * @brief  Lê um byte de um endereço específico da EEPROM.
 */
void EEPROM_ReadByte(uint16_t mem_addr, uint8_t *data)
{
    unsigned char addr_buf[2];
    
    // Define o ponteiro de endereço na EEPROM
    addr_buf[0] = (uint8_t)(mem_addr >> 8);   // MSB
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF); // LSB

    // 1. Dummy Write para configurar o endereço atual de leitura
    i2cSend(EEPROM_I2C_ADDR, addr_buf, 2);

    // Pequeno atraso para estabilização do barramento
    for (volatile uint32_t i = 0; i < 200; i++);

    // 2. Recebe o byte da EEPROM
    i2cReceive(EEPROM_I2C_ADDR, data, 1);
}

/**
 * @brief  Escreve múltiplos bytes (página) na EEPROM respeitando limites de página de 128 bytes.
 */
void EEPROM_WritePage(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    while (len > 0) {
        // Calcula o limite da página atual
        uint16_t page_offset = mem_addr % EEPROM_PAGE_SIZE;
        uint16_t bytes_to_end = EEPROM_PAGE_SIZE - page_offset;
        uint16_t chunk = (len < bytes_to_end) ? len : bytes_to_end;

        // Buffer temporário para endereço de 16 bits + dados do bloco
        unsigned char buffer[130]; // 2 bytes de endereço + até 128 bytes de dados
        buffer[0] = (uint8_t)(mem_addr >> 8);
        buffer[1] = (uint8_t)(mem_addr & 0xFF);
        for (uint16_t i = 0; i < chunk; i++) {
            buffer[2 + i] = data[i];
        }

        // Transmite o bloco de dados
        i2cSend(EEPROM_I2C_ADDR, buffer, 2 + chunk);

        // Aguarda ciclo de escrita
        for (volatile uint32_t i = 0; i < 30000; i++);

        // Avança ponteiros
        mem_addr += chunk;
        data += chunk;
        len -= chunk;
    }
}

/**
 * @brief  Lê múltiplos bytes de forma sequencial na EEPROM.
 */
void EEPROM_ReadSequential(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    unsigned char addr_buf[2];
    
    // Configura o ponteiro de endereço inicial
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF);

    // 1. Dummy Write para definir endereço
    i2cSend(EEPROM_I2C_ADDR, addr_buf, 2);

    // Pequeno atraso para estabilização
    for (volatile uint32_t i = 0; i < 200; i++);

    // 2. Recebe a sequência de bytes de uma vez só
    i2cReceive(EEPROM_I2C_ADDR, data, len);
}
