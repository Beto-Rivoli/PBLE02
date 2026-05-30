/*****************************************************************************
 * lcd.h
 *
 * Driver para display LCD 16x2 (HD44780) em modo 4 bits
 * no microcontrolador LPC11U14 (LQFP48).
 *
 * Mapeamento de Hardware (tudo na Porta 1):
 *   RS (Register Select) : PIO1_31
 *   EN (Enable)          : PIO1_28
 *   D4                   : PIO1_19
 *   D5                   : PIO1_20
 *   D6                   : PIO1_21
 *   D7                   : PIO1_22
 *
 *****************************************************************************/

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

/*===========================================================================
 * Definicoes dos pinos (numero do bit na Porta 1)
 *===========================================================================*/
#define LCD_RS_PIN   31   /* PIO1_31 - Register Select */
#define LCD_EN_PIN   28   /* PIO1_28 - Enable          */
#define LCD_D4_PIN   19   /* PIO1_19 - Data bit 4      */
#define LCD_D5_PIN   20   /* PIO1_20 - Data bit 5      */
#define LCD_D6_PIN   21   /* PIO1_21 - Data bit 6      */
#define LCD_D7_PIN   22   /* PIO1_22 - Data bit 7      */

/*===========================================================================
 * Comandos comuns do HD44780
 *===========================================================================*/
#define LCD_CMD_CLEAR       0x01   /* Limpar display                      */
#define LCD_CMD_HOME        0x02   /* Cursor no inicio                    */
#define LCD_CMD_ENTRY_MODE  0x06   /* Incrementar cursor, sem shift       */
#define LCD_CMD_DISP_ON     0x0C   /* Display ON, cursor OFF, blink OFF   */
#define LCD_CMD_DISP_OFF    0x08   /* Display OFF                         */
#define LCD_CMD_4BIT_2LINE  0x28   /* 4 bits, 2 linhas, 5x8 dots         */
#define LCD_CMD_SET_DDRAM   0x80   /* Set DDRAM address (OR com endereco) */

/*===========================================================================
 * Prototipos de funcoes
 *===========================================================================*/

/**
 * @brief Inicializa o LCD em modo 4 bits.
 *        Configura IOCON, GPIO e executa sequencia de inicializacao HD44780.
 */
void LCD_Init(void);

/**
 * @brief Envia um comando para o LCD (RS = LOW).
 * @param cmd Byte de comando HD44780.
 */
void LCD_Command(uint8_t cmd);

/**
 * @brief Envia um caractere para o LCD (RS = HIGH).
 * @param data Caractere ASCII a ser exibido.
 */
void LCD_Char(char data);

/**
 * @brief Imprime uma string completa no LCD.
 * @param str Ponteiro para string terminada em '\0'.
 */
void LCD_String(const char *str);

/**
 * @brief Posiciona o cursor no LCD.
 * @param row Linha (0 ou 1).
 * @param col Coluna (0 a 15).
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief Limpa o display e retorna cursor ao inicio.
 */
void LCD_Clear(void);

/**
 * @brief Retorna o cursor ao inicio (sem limpar).
 */
void LCD_Home(void);

/**
 * @brief Imprime um numero inteiro no LCD.
 * @param num Valor inteiro a ser exibido.
 */
void LCD_Number(int num);

#endif /* LCD_H */