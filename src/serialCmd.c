/*****************************************************************************
 * serialCmd.c
 *
 * Módulo de parser de comandos seriais — implementação.
 * Acumula bytes do serialReadChar() em buffer até '\n', depois faz parse
 * e retorna o event_t correspondente.
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#include "serialCmd.h"
#include "serial.h"
#include "var.h"

/*===========================================================================
 * Buffer de recepção
 *===========================================================================*/
#define RX_BUF_SIZE     32

static char rx_buf[RX_BUF_SIZE];
static uint8_t rx_idx = 0;

/*===========================================================================
 * Variável global para valor de parâmetro serial
 * Acessível pelo stateMachine para aplicar o valor setado.
 *===========================================================================*/
volatile uint16_t serial_param_value = 0;

/*===========================================================================
 * Funções auxiliares internas
 *===========================================================================*/

/**
 * @brief Converte string decimal (até 4 dígitos) em uint16_t.
 */
static uint16_t parseDecimal(const char *str, uint8_t len)
{
    uint16_t val = 0;
    uint8_t i;
    for (i = 0; i < len; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            val = val * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    return val;
}

/**
 * @brief Converte uint16_t em string decimal (4 dígitos, zero-padded).
 */
static void formatDecimal(uint16_t val, char *buf)
{
    buf[0] = '0' + (val / 1000) % 10;
    buf[1] = '0' + (val / 100) % 10;
    buf[2] = '0' + (val / 10) % 10;
    buf[3] = '0' + val % 10;
    buf[4] = '\0';
}

/**
 * @brief Faz parse do buffer e retorna o evento correspondente.
 */
static event_t parseCommand(void)
{
    if (rx_idx == 0) return EVT_NONE;

    char cmd = rx_buf[0];

    switch (cmd) {
        case 'f':
        case 'F':
            return EVT_SERIAL_FWD;

        case 'a':
        case 'A':
            return EVT_SERIAL_STATUS;

        case 'H':
        case 'h':
            if (rx_idx > 1) {
                serial_param_value = parseDecimal(&rx_buf[1], rx_idx - 1);
                if (serial_param_value > 1023) serial_param_value = 1023;
                return EVT_SERIAL_SET_HI;
            }
            break;

        case 'L':
        case 'l':
            if (rx_idx > 1) {
                serial_param_value = parseDecimal(&rx_buf[1], rx_idx - 1);
                if (serial_param_value > 1023) serial_param_value = 1023;
                return EVT_SERIAL_SET_LO;
            }
            break;

        case 'I':
        case 'i':
            if (rx_idx > 1) {
                serial_param_value = parseDecimal(&rx_buf[1], rx_idx - 1);
                if (serial_param_value > 1) serial_param_value = 0;
                return EVT_SERIAL_SET_LANG;
            }
            break;

        default:
            break;
    }

    return EVT_NONE;
}

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void SCMD_Init(void)
{
    rx_idx = 0;
    serial_param_value = 0;
}

event_t SCMD_Poll(void)
{
    int ch = serialReadChar();

    while (ch != -1) {
        if (ch == '\n' || ch == '\r') {
            /* Fim de comando — fazer parse */
            if (rx_idx > 0) {
                event_t evt = parseCommand();
                rx_idx = 0;  /* Reset do buffer */
                return evt;
            }
            rx_idx = 0;
        } else {
            /* Acumular no buffer */
            if (rx_idx < RX_BUF_SIZE - 1) {
                rx_buf[rx_idx++] = (char)ch;
            }
            /* Se buffer cheio, descartar (evitar overflow) */
        }

        ch = serialReadChar();
    }

    return EVT_NONE;
}

void SCMD_SendStatus(uint16_t adcVal, uint8_t alarmOn,
                     uint16_t hi, uint16_t lo, uint8_t lang)
{
    char numBuf[5];

    serialSendString("--- STATUS ---\r\n");

    serialSendString("ADC: ");
    formatDecimal(adcVal, numBuf);
    serialSendString(numBuf);
    serialSendString("\r\n");

    serialSendString("Alarm: ");
    serialSendString(alarmOn ? "ON" : "OFF");
    serialSendString("\r\n");

    serialSendString("Hi: ");
    formatDecimal(hi, numBuf);
    serialSendString(numBuf);
    serialSendString("  Lo: ");
    formatDecimal(lo, numBuf);
    serialSendString(numBuf);
    serialSendString("\r\n");

    serialSendString("Lang: ");
    serialSendString(lang == 0 ? "PT" : "EN");
    serialSendString("\r\n");

    serialSendString("--------------\r\n");
}
