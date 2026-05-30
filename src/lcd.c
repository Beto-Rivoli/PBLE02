/*****************************************************************************
 * lcd.c
 *
 * Driver para display LCD 16x2 (HD44780) em modo 4 bits
 * no microcontrolador LPC11U14 (LQFP48).
 *
 * Todos os pinos estao na Porta 1, usando acesso direto CMSIS:
 *   RS = PIO1_31, EN = PIO1_28
 *   D4 = PIO1_19, D5 = PIO1_20, D6 = PIO1_21, D7 = PIO1_22
 *
 * NOTA: PIO1_31 usa bit 31, entao TODOS os shifts usam 1U (unsigned)
 *       para evitar comportamento indefinido com (1 << 31) em signed int.
 *
 *****************************************************************************/

#include "LPC11Uxx.h"
#include "lcd.h"

/*===========================================================================
 *  Mascara de todos os pinos do LCD (para operacoes em lote)
 *  IMPORTANTE: usar 1U para evitar UB em (1 << 31) com int signed
 *===========================================================================*/
#define LCD_ALL_PINS  ( (1U << LCD_RS_PIN) | (1U << LCD_EN_PIN) | \
                        (1U << LCD_D4_PIN) | (1U << LCD_D5_PIN) | \
                        (1U << LCD_D6_PIN) | (1U << LCD_D7_PIN) )

/* Mascara somente dos pinos de dados D4..D7 */
#define LCD_DATA_MASK ( (1U << LCD_D4_PIN) | (1U << LCD_D5_PIN) | \
                        (1U << LCD_D6_PIN) | (1U << LCD_D7_PIN) )

/*===========================================================================
 *  Funcoes de delay por loop (estimativas para ~48 MHz sem otimizacao)
 *  Ajuste o multiplicador conforme necessario para o seu clock/compilador.
 *===========================================================================*/

static void LCD_DelayUs(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < (us * 12); i++);   /* Aumentado para garantir delay seguro de ~1.5us por unidade @ 48 MHz */
}

static void LCD_DelayMs(uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < ms; i++) {
        LCD_DelayUs(1000);
    }
}

/*===========================================================================
 *  LCD_Pulse()
 *  Gera pulso de Enable: LOW -> HIGH -> delay -> LOW
 *===========================================================================*/
static void LCD_Pulse(void)
{
    LPC_GPIO->SET[1] = (1U << LCD_EN_PIN);   /* EN = HIGH */
    LCD_DelayUs(5);                           /* Manter alto ~5 us */
    LPC_GPIO->CLR[1] = (1U << LCD_EN_PIN);   /* EN = LOW  */
    LCD_DelayUs(5);                           /* Hold time */
}

/*===========================================================================
 *  LCD_SendNibble(nibble)
 *  Coloca os 4 bits do nibble nos pinos D4..D7 e pulsa EN.
 *
 *  nibble[0] -> PIO1_19 (D4)
 *  nibble[1] -> PIO1_20 (D5)
 *  nibble[2] -> PIO1_21 (D6)
 *  nibble[3] -> PIO1_22 (D7)
 *===========================================================================*/
static void LCD_SendNibble(uint8_t nibble)
{
    /* Limpar os 4 pinos de dados */
    LPC_GPIO->CLR[1] = LCD_DATA_MASK;

    /* Setar os bits que devem ser 1 */
    if (nibble & 0x01) LPC_GPIO->SET[1] = (1U << LCD_D4_PIN);
    if (nibble & 0x02) LPC_GPIO->SET[1] = (1U << LCD_D5_PIN);
    if (nibble & 0x04) LPC_GPIO->SET[1] = (1U << LCD_D6_PIN);
    if (nibble & 0x08) LPC_GPIO->SET[1] = (1U << LCD_D7_PIN);

    /* Pulsar Enable para transferir o nibble */
    LCD_Pulse();
}

/*===========================================================================
 *  LCD_Command(cmd)
 *  Envia um comando para o LCD (RS = LOW).
 *  O byte e dividido em dois nibbles: MSB primeiro, depois LSB.
 *===========================================================================*/
void LCD_Command(uint8_t cmd)
{
    LPC_GPIO->CLR[1] = (1U << LCD_RS_PIN);   /* RS = LOW (comando) */
    LCD_SendNibble(cmd >> 4);                 /* Nibble alto */
    LCD_SendNibble(cmd & 0x0F);               /* Nibble baixo */
    LCD_DelayMs(2);                           /* Tempo de execucao do comando */
}

/*===========================================================================
 *  LCD_Char(data)
 *  Envia um caractere para o LCD (RS = HIGH).
 *===========================================================================*/
void LCD_Char(char data)
{
    LPC_GPIO->SET[1] = (1U << LCD_RS_PIN);   /* RS = HIGH (dado) */
    LCD_SendNibble((uint8_t)data >> 4);       /* Nibble alto */
    LCD_SendNibble((uint8_t)data & 0x0F);     /* Nibble baixo */
    LCD_DelayMs(2);                           /* Tempo de execucao */
}

/*===========================================================================
 *  LCD_String(str)
 *  Imprime uma string completa no LCD.
 *===========================================================================*/
void LCD_String(const char *str)
{
    while (*str) {
        LCD_Char(*str++);
    }
}

/*===========================================================================
 *  LCD_Clear()
 *  Limpa o display e retorna cursor ao inicio.
 *===========================================================================*/
void LCD_Clear(void)
{
    LCD_Command(LCD_CMD_CLEAR);
    LCD_DelayMs(2);   /* Clear display demora ~1.52 ms */
}

/*===========================================================================
 *  LCD_Home()
 *  Retorna cursor ao inicio sem limpar o display.
 *===========================================================================*/
void LCD_Home(void)
{
    LCD_Command(LCD_CMD_HOME);
    LCD_DelayMs(2);   /* Return home demora ~1.52 ms */
}

/*===========================================================================
 *  LCD_SetCursor(row, col)
 *  Posiciona o cursor no LCD 16x2.
 *    row = 0 -> primeira linha  (endereco base 0x00)
 *    row = 1 -> segunda linha   (endereco base 0x40)
 *===========================================================================*/
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    const uint8_t row_offsets[] = { 0x00, 0x40 };
    if (row > 1) row = 1;
    if (col > 15) col = 15;
    LCD_Command(LCD_CMD_SET_DDRAM | (row_offsets[row] + col));
}

/*===========================================================================
 *  LCD_Number(num)
 *  Imprime um numero inteiro no LCD.
 *  Suporta numeros negativos e zero.
 *===========================================================================*/
void LCD_Number(int num)
{
    char buf[12];   /* Suficiente para int32 (-2147483648) */
    int i = 0;
    int neg = 0;

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    if (num == 0) {
        LCD_Char('0');
        return;
    }

    /* Extrair digitos (ordem reversa) */
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (neg) {
        LCD_Char('-');
    }

    /* Imprimir digitos na ordem correta */
    while (i > 0) {
        LCD_Char(buf[--i]);
    }
}

/*===========================================================================
 *  LCD_Init()
 *
 *  1. Habilita clocks do GPIO e IOCON
 *  2. Configura IOCON dos 6 pinos como GPIO (FUNC=0)
 *  3. Configura os 6 pinos como saida digital
 *  4. Executa sequencia de inicializacao HD44780 em modo 4 bits
 *===========================================================================*/
void LCD_Init(void)
{
    /*-----------------------------------------------------------------------
     *  1. Habilitar clocks necessarios:
     *     Bit 6  = GPIO  (normalmente ja habilitado apos reset)
     *     Bit 16 = IOCON (configuracao de pinos)
     *-----------------------------------------------------------------------*/
    LPC_SYSCON->SYSAHBCLKCTRL |= (1U << 6) | (1U << 16);

    /*-----------------------------------------------------------------------
     *  2. Configurar registradores IOCON dos 6 pinos do LCD
     *     Todos como: FUNC=0 (GPIO), MODE=00 (sem pull), HYS=0, OD=0
     *     Valor = 0x00
     *-----------------------------------------------------------------------*/
    LPC_IOCON->PIO1_31 = 0x00;   /* RS  */
    LPC_IOCON->PIO1_28 = 0x00;   /* EN  */
    LPC_IOCON->PIO1_19 = 0x00;   /* D4  */
    LPC_IOCON->PIO1_20 = 0x00;   /* D5  */
    LPC_IOCON->PIO1_21 = 0x00;   /* D6  */
    LPC_IOCON->PIO1_22 = 0x00;   /* D7  */

    /*-----------------------------------------------------------------------
     *  3. Configurar os 6 pinos como SAIDA na Porta 1
     *     Setar os bits correspondentes em LPC_GPIO->DIR[1]
     *-----------------------------------------------------------------------*/
    LPC_GPIO->DIR[1] |= LCD_ALL_PINS;

    /* Garantir que todos os pinos comecam em LOW */
    LPC_GPIO->CLR[1] = LCD_ALL_PINS;

    /*-----------------------------------------------------------------------
     *  4. Sequencia de inicializacao HD44780 - Modo 4 bits
     *
     *     Conforme datasheet HD44780:
     *     - Aguardar >40 ms apos Vcc subir para 2.7V
     *     - Enviar 0x3 tres vezes (interface 8 bits, forcar estado conhecido)
     *     - Enviar 0x2 (trocar para interface 4 bits)
     *     - Configurar: 4 bits, 2 linhas, 5x8 dots
     *     - Display ON, Cursor OFF, Blink OFF
     *     - Entry Mode: incrementar cursor, sem shift
     *     - Clear Display
     *-----------------------------------------------------------------------*/

    /* Aguardar power-on do LCD (aumentado para 300 ms para garantir estabilizacao da tensao de alimentacao) */
    LCD_DelayMs(300);

    /* RS = LOW para modo comando durante toda a inicializacao */
    LPC_GPIO->CLR[1] = (1U << LCD_RS_PIN);

    /* Passo 1: Enviar 0x3 (Function Set - 8 bit) - primeira vez */
    LCD_SendNibble(0x03);
    LCD_DelayMs(5);            /* Aguardar >4.1 ms */

    /* Passo 2: Enviar 0x3 - segunda vez */
    LCD_SendNibble(0x03);
    LCD_DelayUs(200);          /* Aguardar >100 us */

    /* Passo 3: Enviar 0x3 - terceira vez */
    LCD_SendNibble(0x03);
    LCD_DelayUs(200);

    /* Passo 4: Enviar 0x2 (trocar para modo 4 bits) */
    LCD_SendNibble(0x02);
    LCD_DelayMs(5);

    /*--- A partir daqui, comunicacao ja e em modo 4 bits (2 nibbles) ---*/

    /* Function Set: 4 bits, 2 linhas, fonte 5x8 */
    LCD_Command(LCD_CMD_4BIT_2LINE);    /* 0x28 */

    /* Display ON, Cursor OFF, Blink OFF */
    LCD_Command(LCD_CMD_DISP_ON);       /* 0x0C */

    /* Entry Mode: incrementar cursor, sem shift do display */
    LCD_Command(LCD_CMD_ENTRY_MODE);    /* 0x06 */

    /* Limpar display */
    LCD_Clear();
}
