/*****************************************************************************
 * alarm.c
 *
 * Módulo de lógica de alarme — implementação.
 * Usa o módulo var para obter limites e o PWM/GPIO para atuadores.
 *
 * Atuadores:
 *   Buzzer — PWM em PIO0_18 (CT32B0_MAT0), duty 50% quando ativo
 *   LED    — PIO1_24 (LED0), HIGH quando ativo
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#include "alarm.h"
#include "var.h"
#include "pwm.h"
#include "io.h"

/*===========================================================================
 * Estado interno
 *===========================================================================*/
static uint8_t alarm_active = 0;

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void ALARM_Init(void)
{
    /* Configurar pino do LED como saída */
    pinMode(LED0, OUTPUT);
    digitalWrite(LED0, LOW);

    /* Garantir que PWM inicia em 0% */
    pwmSetDuty(0);

    alarm_active = 0;
}

uint8_t ALARM_Check(uint16_t adcVal)
{
    uint16_t hi = VAR_GetAlarmHi();
    uint16_t lo = VAR_GetAlarmLo();

    if (adcVal > hi) {
        return ALARM_HI;
    }
    if (adcVal < lo) {
        return ALARM_LO;
    }
    return ALARM_OK;
}

void ALARM_Activate(void)
{
    if (!alarm_active) {
        alarm_active = 1;
        pwmSetDuty(500);            /* 50% duty cycle para buzzer     */
        digitalWrite(LED0, HIGH);   /* LED indicador de alarme        */
    }
}

void ALARM_Deactivate(void)
{
    if (alarm_active) {
        alarm_active = 0;
        pwmSetDuty(0);              /* Desliga buzzer                 */
        digitalWrite(LED0, LOW);    /* Desliga LED                    */
    }
}

uint8_t ALARM_IsActive(void)
{
    return alarm_active;
}
