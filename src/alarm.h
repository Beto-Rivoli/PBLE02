/*****************************************************************************
 * alarm.h
 *
 * Módulo de lógica de alarme — verifica limites ADC e controla
 * atuadores (buzzer via PWM, LED via GPIO).
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

/*===========================================================================
 * Resultado da verificação de alarme
 *===========================================================================*/
#define ALARM_OK        0   /* ADC dentro dos limites               */
#define ALARM_HI        1   /* ADC acima do limite superior         */
#define ALARM_LO        2   /* ADC abaixo do limite inferior        */

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Inicializa o módulo de alarme.
 *         Configura o pino do LED como saída e garante que
 *         alarme começa desativado.
 */
void ALARM_Init(void);

/**
 * @brief  Verifica se o valor ADC está fora dos limites configurados.
 * @param  adcVal  Valor ADC lido (0–1023).
 * @return ALARM_OK, ALARM_HI ou ALARM_LO.
 */
uint8_t ALARM_Check(uint16_t adcVal);

/**
 * @brief  Ativa o alarme: liga buzzer (PWM 50%) e LED indicador.
 */
void ALARM_Activate(void);

/**
 * @brief  Desativa o alarme: desliga buzzer e LED.
 */
void ALARM_Deactivate(void);

/**
 * @brief  Retorna se o alarme está ativo.
 * @return 1 se ativo, 0 se inativo.
 */
uint8_t ALARM_IsActive(void);

#endif /* ALARM_H */
