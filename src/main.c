/*****************************************************************************
 * main.c
 *
 * Central de Alarme — Super-loop ESM (Embedded State Machine)
 *
 * Fluxo principal:
 *   1. Inicializa todos os periféricos e módulos
 *   2. Configura SysTick para tick de 10ms
 *   3. Super-loop:
 *      a) Coleta eventos (encoder, teclas, serial, alarme)
 *      b) Processa cada evento na ESM
 *      c) Executa ações de saída (LCD, alarme, serial, EEPROM)
 *
 * Microcontrolador: LPC11U14 (ARM Cortex-M0, 48 MHz)
 *
 *****************************************************************************/

#include "LPC11Uxx.h"
#include "system_LPC11Uxx.h"

/* Drivers (HAL) */
#include "inputs.h"
#include "lcd.h"
#include "eeprom_24lc512.h"
#include "adc.h"
#include "pwm.h"
#include "serial.h"
#include "io.h"

/* Kernel (ESM) */
#include "stateMachine.h"

/* Aplicação */
#include "var.h"
#include "alarm.h"
#include "display.h"
#include "serialCmd.h"
#include "bod.h"

/*===========================================================================
 * Configuração do SysTick
 * SysTick gera interrupção a cada 10ms (100 Hz)
 * SystemCoreClock = 48000000 Hz
 * Reload = 48000000 / 100 = 480000
 *===========================================================================*/
#define SYSTICK_RELOAD  (SystemCoreClock / 100)  /* 10ms */

/*===========================================================================
 * Flags de sistema (setadas pelo SysTick, lidas pelo super-loop)
 *===========================================================================*/
static volatile uint8_t tick_flag = 0;       /* Flag de tick 10ms         */
static volatile uint8_t display_tick = 0;    /* Contador para refresh LCD */

#define DISPLAY_REFRESH_TICKS   20  /* 200ms entre atualizações de LCD   */

/*===========================================================================
 * SysTick Handler
 * Chamado a cada 10ms pelo hardware SysTick.
 *===========================================================================*/
void SysTick_Handler(void)
{
    tick_flag = 1;

    /* Debounce das entradas */
    inputs_tick();

    /* Cronômetro da ESM */
    ESM_TickChrono();

    /* Contador de refresh do display */
    display_tick++;
}

/*===========================================================================
 * Função principal
 *===========================================================================*/
int main(void)
{
    event_t evt;
    output_t action;
    uint16_t current_adc;
    uint8_t alarm_status;

    /*------------------------------------------------------------------
     * 1. Inicialização dos periféricos (Camada Driver)
     *------------------------------------------------------------------*/
    SystemInit();
    init_inputs();             /* Teclado (KEY1) e encoder            */
    LCD_Init();                /* Display LCD 16x2                    */
    I2C_EEPROM_Init();         /* Barramento I2C + EEPROM 24LC512     */
    adcInit();                 /* Conversor ADC (canal AD1)           */
    pwmInit();                 /* PWM para buzzer (PIO0_18)           */
    serialInit();              /* UART 9600 8N1                       */

    /*------------------------------------------------------------------
     * 2. Inicialização dos módulos (Camada Aplicação)
     *------------------------------------------------------------------*/
    BOD_Init();                /* Brown-Out Detect (segurança)        */
    VAR_Init();                /* Variáveis persistentes (EEPROM)     */
    ALARM_Init();              /* Lógica de alarme (LED + buzzer)     */
    SCMD_Init();               /* Parser de comandos seriais          */

    /*------------------------------------------------------------------
     * 3. Inicialização do Kernel (ESM)
     *------------------------------------------------------------------*/
    ESM_Init();                /* Máquina de estados (carrega estado) */

    /*------------------------------------------------------------------
     * 4. Tela de inicialização (diagnóstico rápido)
     *------------------------------------------------------------------*/
    {
        uint8_t test_val = 0x00;
        EEPROM_WriteByte(0x0010, 0xA5);
        EEPROM_ReadByte(0x0010, &test_val);

        LCD_SetCursor(0, 0);
        if (test_val == 0xA5) {
            LCD_String("Central Alarme  ");
            LCD_SetCursor(1, 0);
            LCD_String("EEPROM: OK      ");
        } else {
            LCD_String("Central Alarme  ");
            LCD_SetCursor(1, 0);
            LCD_String("EEPROM: ERRO!   ");
        }

        /* Aguardar ~1.5 segundos */
        for (volatile uint32_t w = 0; w < 1500000; w++);
    }

    /*------------------------------------------------------------------
     * 5. Configurar SysTick para 10ms (após toda inicialização)
     *------------------------------------------------------------------*/
    SysTick_Config(SYSTICK_RELOAD);

    /*------------------------------------------------------------------
     * 6. Primeira atualização do display
     *------------------------------------------------------------------*/
    current_adc = adcRead();
    ESM_UpdateADC(current_adc);
    ESM_UpdateDisplay();

    /*==================================================================
     * 7. SUPER-LOOP PRINCIPAL
     *==================================================================*/
    while (1)
    {
        /*--------------------------------------------------------------
         * 7a. Aguardar tick de 10ms (economia de energia)
         *--------------------------------------------------------------*/
        if (!tick_flag) {
            __WFI();  /* Wait For Interrupt — dorme até próximo tick */
            continue;
        }
        tick_flag = 0;

        /*--------------------------------------------------------------
         * 7b. Leitura contínua do ADC
         *--------------------------------------------------------------*/
        current_adc = adcRead();
        ESM_UpdateADC(current_adc);

        /*--------------------------------------------------------------
         * 7c. Verificação de alarme ADC
         *--------------------------------------------------------------*/
        alarm_status = ALARM_Check(current_adc);

        if (alarm_status != ALARM_OK && !ALARM_IsActive()) {
            /* Disparar alarme */
            ALARM_Activate();

            /* Registrar evento no log */
            alarmLog_t log;
            log.timestamp = ESM_GetChronoSeconds();
            log.adcValue  = current_adc;
            log.alarmType = alarm_status;
            log.reserved  = 0;
            VAR_LogEvent(log);
            VAR_Save();

            /* Processar evento de alarme na ESM */
            action = ESM_ProcessEvent(EVT_ALARM_TRIGGER);
            if (action == OUT_LCD_UPDATE) {
                ESM_UpdateDisplay();
            }
        }
        else if (alarm_status == ALARM_OK && ALARM_IsActive()) {
            /* Alarme foi resolvido */
            ALARM_Deactivate();

            action = ESM_ProcessEvent(EVT_ALARM_CLEAR);
            if (action == OUT_LCD_UPDATE) {
                ESM_UpdateDisplay();
            }
        }

        /*--------------------------------------------------------------
         * 7d. Coletar e processar eventos de entrada (encoder + teclas)
         *--------------------------------------------------------------*/
        evt = inputs_getEvent();
        if (evt != EVT_NONE) {
            action = ESM_ProcessEvent(evt);

            switch (action) {
                case OUT_LCD_UPDATE:
                    ESM_UpdateDisplay();
                    break;

                case OUT_ALARM_ON:
                    ALARM_Activate();
                    ESM_UpdateDisplay();
                    break;

                case OUT_ALARM_OFF:
                    ALARM_Deactivate();
                    ESM_UpdateDisplay();
                    break;

                case OUT_SAVE_VARS:
                    VAR_Save();
                    ESM_UpdateDisplay();
                    break;

                case OUT_SEND_STATUS:
                    SCMD_SendStatus(current_adc, ALARM_IsActive(),
                                    VAR_GetAlarmHi(), VAR_GetAlarmLo(),
                                    VAR_GetLanguage());
                    break;

                default:
                    break;
            }
        }

        /*--------------------------------------------------------------
         * 7e. Coletar e processar comandos seriais
         *--------------------------------------------------------------*/
        evt = SCMD_Poll();
        if (evt != EVT_NONE) {
            action = ESM_ProcessEvent(evt);

            switch (action) {
                case OUT_LCD_UPDATE:
                    ESM_UpdateDisplay();
                    break;

                case OUT_SAVE_VARS:
                    VAR_Save();
                    ESM_UpdateDisplay();
                    break;

                case OUT_SEND_STATUS:
                    SCMD_SendStatus(current_adc, ALARM_IsActive(),
                                    VAR_GetAlarmHi(), VAR_GetAlarmLo(),
                                    VAR_GetLanguage());
                    break;

                default:
                    break;
            }
        }

        /*--------------------------------------------------------------
         * 7f. Refresh periódico do display (200ms para telas dinâmicas)
         *--------------------------------------------------------------*/
        if (display_tick >= DISPLAY_REFRESH_TICKS) {
            display_tick = 0;

            state_t st = ESM_GetState();
            /* Atualizar apenas telas que mudam continuamente */
            if (st == ST_SUB_ADC_VIEW ||
                st == ST_SUB_CTRL_VIEW ||
                st == ST_EDIT_CLOCK)
            {
                action = ESM_ProcessEvent(EVT_TICK);
                if (action == OUT_LCD_UPDATE) {
                    ESM_UpdateDisplay();
                }
            }
        }
    }

    return 0;
}
