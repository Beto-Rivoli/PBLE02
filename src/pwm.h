#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/**
 * @brief Inicializa o temporizador CT32B0 para gerar um sinal PWM
 *        com frequencia de 1 kHz no pino PIO0_18 (pino fisico 11).
 */
void pwmInit(void);

/**
 * @brief Define o ciclo ativo (duty cycle) do PWM.
 * @param duty Valor de 0 a 1000 (onde 0 = desligado, 1000 = 100% ligado).
 */
void pwmSetDuty(uint32_t duty);

#endif // PWM_H
