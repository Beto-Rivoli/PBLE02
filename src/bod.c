/*****************************************************************************
 * bod.c
 *
 * Módulo de detecção de Brown-Out (BOD) — implementação.
 *
 * Na interrupção BOD:
 *   1. Desativa alarme (buzzer/LED) para reduzir consumo
 *   2. Salva variáveis críticas na EEPROM (salvamento emergencial)
 *   3. Entra em loop infinito aguardando reset por hardware
 *
 * Camada: Driver / Segurança
 *
 *****************************************************************************/

#include "bod.h"
#include "LPC11Uxx.h"
#include "var.h"
#include "alarm.h"

/*===========================================================================
 * Definições de bits do BODCTRL
 *===========================================================================*/
#define BODRSTLEV_2V35      (0x02 << 0)  /* Reset level 2: 2.35V       */
#define BODINTVAL_2V80      (0x03 << 2)  /* Interrupt level 3: 2.80V   */
#define BODRSTENA           (1 << 4)     /* Habilita reset por BOD     */

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void BOD_Init(void)
{
    /*
     * Configurar BODCTRL:
     *   - BODRSTLEV  = 10 (2.35V) — Reset se tensão cair a este nível
     *   - BODINTVAL  = 11 (2.80V) — Interrupção ao cair a este nível
     *   - BODRSTENA  = 1          — Habilitar reset por BOD
     *
     * Assim, quando Vdd cai a 2.80V, a ISR é chamada para salvar dados.
     * Se Vdd continuar caindo até 2.35V, o hardware faz reset forçado.
     */
    LPC_SYSCON->BODCTRL = BODRSTLEV_2V35 | BODINTVAL_2V80 | BODRSTENA;

    /* Habilitar interrupção BOD no NVIC (prioridade alta) */
    NVIC_SetPriority(BOD_IRQn, 0);  /* Prioridade máxima */
    NVIC_EnableIRQ(BOD_IRQn);
}

/*===========================================================================
 * ISR do Brown-Out Detect
 *
 * O nome BOD_IRQHandler corresponde ao vetor BOD_IRQn (26) no startup.
 * Declarado como WEAK no cr_startup_lpc11uxx.c, portanto esta definição
 * sobrescreve o handler padrão.
 *===========================================================================*/

void BOD_IRQHandler(void)
{
    /* 1. Desativar alarme para reduzir consumo de corrente */
    ALARM_Deactivate();

    /* 2. Salvamento emergencial das variáveis críticas na EEPROM */
    VAR_SaveEmergency();

    /*
     * 3. Entrar em loop infinito.
     *    Se a tensão se recuperar, o sistema continua rodando normalmente
     *    após o retorno da ISR. Se a tensão continuar caindo, o reset
     *    por hardware (BODRSTENA a 2.35V) será acionado.
     *
     *    Usamos __WFI() para minimizar consumo enquanto aguardamos.
     */
    while (1) {
        __WFI();  /* Wait For Interrupt — baixo consumo */
    }
}
