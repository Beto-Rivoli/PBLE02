/*****************************************************************************
 * stateMachine.c
 *
 * Kernel ESM (Embedded State Machine) — implementação.
 * Motor da máquina de estados com tabela de handlers por estado.
 *
 * Cada handler recebe um evento e retorna uma estrutura transition_t
 * contendo o próximo estado e a ação de saída.
 *
 * Variáveis de contexto (cursor de edição, valores temporários) são
 * mantidas como estáticas neste módulo.
 *
 * Camada: Kernel
 *
 *****************************************************************************/

#include "stateMachine.h"
#include "var.h"
#include "alarm.h"
#include "display.h"
#include "serialCmd.h"
#include "adc.h"

/*===========================================================================
 * Estado atual e contexto
 *===========================================================================*/
static state_t currentState = ST_INIT;

/* Contexto de edição de limites */
static uint16_t edit_hi = 0;
static uint16_t edit_lo = 0;
static uint8_t  edit_cursor = 0;   /* 0 = Hi, 1 = Lo */

/* Contexto de edição de idioma */
static uint8_t edit_lang = 0;

/* Cronômetro (contagem de ticks de 10ms) */
static volatile uint32_t chrono_ticks = 0;
static uint8_t chrono_running = 0;

/* Cache do valor ADC atualizado pelo main loop */
static uint16_t cached_adc = 0;

/*===========================================================================
 * Forward declarations dos handlers
 *===========================================================================*/
static transition_t handle_Init(event_t evt);
static transition_t handle_MenuConfig(event_t evt);
static transition_t handle_MenuADC(event_t evt);
static transition_t handle_MenuControl(event_t evt);
static transition_t handle_CfgClock(event_t evt);
static transition_t handle_CfgLimits(event_t evt);
static transition_t handle_CfgLang(event_t evt);
static transition_t handle_EditClock(event_t evt);
static transition_t handle_EditLimits(event_t evt);
static transition_t handle_EditLang(event_t evt);
static transition_t handle_SubADCView(event_t evt);
static transition_t handle_SubCtrlView(event_t evt);

/*===========================================================================
 * Tabela de handlers (indexada por state_t)
 *===========================================================================*/
static const StateHandler stateTable[STATE_COUNT] = {
    [ST_INIT]          = handle_Init,
    [ST_MENU_CONFIG]   = handle_MenuConfig,
    [ST_MENU_ADC]      = handle_MenuADC,
    [ST_MENU_CONTROL]  = handle_MenuControl,
    [ST_CFG_CLOCK]     = handle_CfgClock,
    [ST_CFG_LIMITS]    = handle_CfgLimits,
    [ST_CFG_LANG]      = handle_CfgLang,
    [ST_EDIT_CLOCK]    = handle_EditClock,
    [ST_EDIT_LIMITS]   = handle_EditLimits,
    [ST_EDIT_LANG]     = handle_EditLang,
    [ST_SUB_ADC_VIEW]  = handle_SubADCView,
    [ST_SUB_CTRL_VIEW] = handle_SubCtrlView,
};

/*===========================================================================
 * Macro de transição (conveniência)
 *===========================================================================*/
#define TRANSITION(next, act)   ((transition_t){ (next), (act) })
#define NO_TRANSITION()         ((transition_t){ currentState, OUT_NONE })

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void ESM_Init(void)
{
    /* Tentar recuperar o último estado salvo */
    uint8_t saved = VAR_GetLastState();

    if (saved > 0 && saved < STATE_COUNT) {
        currentState = (state_t)saved;
    } else {
        currentState = ST_MENU_CONFIG;
    }

    /* Inicializar contexto */
    edit_hi = VAR_GetAlarmHi();
    edit_lo = VAR_GetAlarmLo();
    edit_cursor = 0;
    edit_lang = VAR_GetLanguage();
    chrono_ticks = 0;
    chrono_running = 0;
    cached_adc = 0;
}

output_t ESM_ProcessEvent(event_t evt)
{
    if (evt == EVT_NONE) {
        return OUT_NONE;
    }

    /* Garantir que o estado atual é válido */
    if (currentState >= STATE_COUNT || stateTable[currentState] == 0) {
        currentState = ST_MENU_CONFIG;
        return OUT_LCD_UPDATE;
    }

    /* Despachar para o handler do estado atual */
    transition_t trans = stateTable[currentState](evt);

    /* Verificar se houve transição de estado */
    if (trans.nextState != currentState) {
        currentState = trans.nextState;
        VAR_SetLastState((uint8_t)currentState);
    }

    return trans.action;
}

state_t ESM_GetState(void)
{
    return currentState;
}

/*===========================================================================
 * Funções de acesso ao contexto (usadas pelo main loop)
 *===========================================================================*/

/**
 * @brief  Atualiza o valor ADC cacheado (chamado pelo main loop).
 */
void ESM_UpdateADC(uint16_t adcVal)
{
    cached_adc = adcVal;
}

/**
 * @brief  Retorna o valor ADC cacheado.
 */
uint16_t ESM_GetCachedADC(void)
{
    return cached_adc;
}

/**
 * @brief  Incrementa o cronômetro (chamado pelo SysTick a cada 10ms).
 */
void ESM_TickChrono(void)
{
    if (chrono_running) {
        chrono_ticks++;
    }
}

/**
 * @brief  Retorna o tempo do cronômetro em segundos.
 */
uint32_t ESM_GetChronoSeconds(void)
{
    return chrono_ticks / 100;  /* 100 ticks de 10ms = 1 segundo */
}

/*===========================================================================
 * Atualização do display conforme estado atual
 * Chamada pelo main loop após ESM_ProcessEvent retornar OUT_LCD_UPDATE.
 *===========================================================================*/

void ESM_UpdateDisplay(void)
{
    uint8_t lang = VAR_GetLanguage();

    switch (currentState) {
        case ST_MENU_CONFIG:
        case ST_MENU_ADC:
        case ST_MENU_CONTROL:
            DISP_ShowMenu(currentState, lang);
            break;

        case ST_CFG_CLOCK:
        case ST_CFG_LIMITS:
        case ST_CFG_LANG:
            DISP_ShowSubConfig(currentState, lang);
            break;

        case ST_EDIT_CLOCK:
            DISP_ShowClock(ESM_GetChronoSeconds(), lang);
            break;

        case ST_EDIT_LIMITS:
            DISP_ShowEditLimits(edit_hi, edit_lo, edit_cursor, lang);
            break;

        case ST_EDIT_LANG:
            DISP_ShowEditLang(edit_lang);
            break;

        case ST_SUB_ADC_VIEW:
            if (ALARM_IsActive()) {
                uint8_t hiOrLo = (cached_adc > VAR_GetAlarmHi()) ? 1 : 2;
                DISP_ShowAlarmAlert(cached_adc, hiOrLo, lang);
            } else {
                DISP_ShowADC(cached_adc, lang);
            }
            break;

        case ST_SUB_CTRL_VIEW:
            DISP_ShowControl(ALARM_IsActive(), lang);
            break;

        default:
            break;
    }
}

/*===========================================================================
 * Handlers de estado — implementação
 *===========================================================================*/

/* ---- ST_INIT ---- */
static transition_t handle_Init(event_t evt)
{
    (void)evt;
    return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);
}

/* ---- Menu Principal: Configuração ---- */
static transition_t handle_MenuConfig(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_ADC, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_MENU_CONTROL, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        case EVT_SERIAL_SET_HI:
            VAR_SetAlarmHi(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LO:
            VAR_SetAlarmLo(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LANG:
            VAR_SetLanguage((uint8_t)serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Menu Principal: ADC ---- */
static transition_t handle_MenuADC(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_CONTROL, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            return TRANSITION(ST_SUB_ADC_VIEW, OUT_LCD_UPDATE);

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        case EVT_SERIAL_SET_HI:
            VAR_SetAlarmHi(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LO:
            VAR_SetAlarmLo(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LANG:
            VAR_SetLanguage((uint8_t)serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Menu Principal: Controle ---- */
static transition_t handle_MenuControl(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_SERIAL_FWD:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_MENU_ADC, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            return TRANSITION(ST_SUB_CTRL_VIEW, OUT_LCD_UPDATE);

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        case EVT_SERIAL_SET_HI:
            VAR_SetAlarmHi(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LO:
            VAR_SetAlarmLo(serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        case EVT_SERIAL_SET_LANG:
            VAR_SetLanguage((uint8_t)serial_param_value);
            return TRANSITION(currentState, OUT_SAVE_VARS);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Sub-Config: Cronômetro ---- */
static transition_t handle_CfgClock(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
            return TRANSITION(ST_CFG_LIMITS, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_CFG_LANG, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            return TRANSITION(ST_EDIT_CLOCK, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Sub-Config: Limites ---- */
static transition_t handle_CfgLimits(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
            return TRANSITION(ST_CFG_LANG, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            /* Carregar valores atuais para edição */
            edit_hi = VAR_GetAlarmHi();
            edit_lo = VAR_GetAlarmLo();
            edit_cursor = 0;
            return TRANSITION(ST_EDIT_LIMITS, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Sub-Config: Idioma ---- */
static transition_t handle_CfgLang(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            return TRANSITION(ST_CFG_LIMITS, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            edit_lang = VAR_GetLanguage();
            return TRANSITION(ST_EDIT_LANG, OUT_LCD_UPDATE);

        case EVT_KEY1:
            return TRANSITION(ST_MENU_CONFIG, OUT_LCD_UPDATE);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Edição: Cronômetro ---- */
static transition_t handle_EditClock(event_t evt)
{
    switch (evt) {
        case EVT_ENC_SW:
            /* Toggle start/stop do cronômetro */
            chrono_running = !chrono_running;
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_CW:
            /* Sem efeito durante cronômetro (ou poderia resetar) */
            return NO_TRANSITION();

        case EVT_ENC_CCW:
            /* Resetar cronômetro se parado */
            if (!chrono_running) {
                chrono_ticks = 0;
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            }
            return NO_TRANSITION();

        case EVT_KEY1:
            /* Voltar ao sub-menu (sem parar o cronômetro) */
            return TRANSITION(ST_CFG_CLOCK, OUT_LCD_UPDATE);

        case EVT_TICK:
            /* Atualizar display a cada tick (se rodando) */
            if (chrono_running) {
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            }
            return NO_TRANSITION();

        default:
            return NO_TRANSITION();
    }
}

/* ---- Edição: Limites Hi/Lo ---- */
static transition_t handle_EditLimits(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
            /* Incrementar valor selecionado */
            if (edit_cursor == 0) {
                if (edit_hi < 1023) edit_hi += 10;
                if (edit_hi > 1023) edit_hi = 1023;
            } else {
                if (edit_lo < 1023) edit_lo += 10;
                if (edit_lo > 1023) edit_lo = 1023;
            }
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_CCW:
            /* Decrementar valor selecionado */
            if (edit_cursor == 0) {
                if (edit_hi >= 10) edit_hi -= 10;
                else edit_hi = 0;
            } else {
                if (edit_lo >= 10) edit_lo -= 10;
                else edit_lo = 0;
            }
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            if (edit_cursor == 0) {
                /* Mudar para edição de Lo */
                edit_cursor = 1;
                return TRANSITION(currentState, OUT_LCD_UPDATE);
            } else {
                /* Confirmar e salvar */
                VAR_SetAlarmHi(edit_hi);
                VAR_SetAlarmLo(edit_lo);
                edit_cursor = 0;
                return TRANSITION(ST_CFG_LIMITS, OUT_SAVE_VARS);
            }

        case EVT_KEY1:
            /* Cancelar edição e voltar */
            edit_cursor = 0;
            return TRANSITION(ST_CFG_LIMITS, OUT_LCD_UPDATE);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Edição: Idioma ---- */
static transition_t handle_EditLang(event_t evt)
{
    switch (evt) {
        case EVT_ENC_CW:
        case EVT_ENC_CCW:
            /* Toggle entre PT e EN */
            edit_lang = (edit_lang == LANG_PT) ? LANG_EN : LANG_PT;
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            /* Confirmar e salvar */
            VAR_SetLanguage(edit_lang);
            return TRANSITION(ST_CFG_LANG, OUT_SAVE_VARS);

        case EVT_KEY1:
            /* Cancelar e voltar */
            return TRANSITION(ST_CFG_LANG, OUT_LCD_UPDATE);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Visualização: ADC ---- */
static transition_t handle_SubADCView(event_t evt)
{
    switch (evt) {
        case EVT_KEY1:
            return TRANSITION(ST_MENU_ADC, OUT_LCD_UPDATE);

        case EVT_TICK:
            /* Atualizar display com novo valor ADC */
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ALARM_TRIGGER:
            return TRANSITION(currentState, OUT_ALARM_ON);

        case EVT_ALARM_CLEAR:
            return TRANSITION(currentState, OUT_ALARM_OFF);

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        default:
            return NO_TRANSITION();
    }
}

/* ---- Visualização: Controle ---- */
static transition_t handle_SubCtrlView(event_t evt)
{
    switch (evt) {
        case EVT_KEY1:
            return TRANSITION(ST_MENU_CONTROL, OUT_LCD_UPDATE);

        case EVT_ENC_SW:
            /* Toggle manual do alarme (para teste) */
            if (ALARM_IsActive()) {
                ALARM_Deactivate();
            } else {
                ALARM_Activate();
            }
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_TICK:
            return TRANSITION(currentState, OUT_LCD_UPDATE);

        case EVT_ALARM_TRIGGER:
            return TRANSITION(currentState, OUT_ALARM_ON);

        case EVT_ALARM_CLEAR:
            return TRANSITION(currentState, OUT_ALARM_OFF);

        case EVT_SERIAL_STATUS:
            return TRANSITION(currentState, OUT_SEND_STATUS);

        default:
            return NO_TRANSITION();
    }
}
