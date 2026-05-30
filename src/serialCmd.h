/*****************************************************************************
 * serialCmd.h
 *
 * Módulo de parser de comandos seriais — recebe bytes da UART e decodifica
 * em eventos ESM.
 *
 * Protocolo:
 *   'f\n'      — Forward menu        → EVT_SERIAL_FWD
 *   'a\n'      — Status ADC          → EVT_SERIAL_STATUS
 *   'Hxxxx\n'  — Setar limite Hi     → EVT_SERIAL_SET_HI (valor em serial_param_value)
 *   'Lxxxx\n'  — Setar limite Lo     → EVT_SERIAL_SET_LO (valor em serial_param_value)
 *   'Ix\n'     — Setar idioma (0/1)  → EVT_SERIAL_SET_LANG (valor em serial_param_value)
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#ifndef SERIALCMD_H
#define SERIALCMD_H

#include "stateMachine.h"

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Inicializa o módulo de comandos seriais.
 *         Limpa o buffer interno de recepção.
 */
void SCMD_Init(void);

/**
 * @brief  Verifica a UART por novos bytes e tenta decodificar um comando.
 *         Deve ser chamada periodicamente no super-loop.
 * @return Evento correspondente ao comando, ou EVT_NONE se nenhum
 *         comando completo foi recebido.
 */
event_t SCMD_Poll(void);

/**
 * @brief  Envia uma string de status formatada via UART.
 * @param  adcVal    Valor atual do ADC.
 * @param  alarmOn   1 se alarme ativo, 0 caso contrário.
 * @param  hi        Limite superior atual.
 * @param  lo        Limite inferior atual.
 * @param  lang      Idioma configurado.
 */
void SCMD_SendStatus(uint16_t adcVal, uint8_t alarmOn,
                     uint16_t hi, uint16_t lo, uint8_t lang);

#endif /* SERIALCMD_H */
