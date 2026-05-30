/*****************************************************************************
 * stateMachine.h
 *
 * Kernel ESM (Embedded State Machine) para a Central de Alarme.
 * Define os tipos fundamentais (estados, eventos, saídas) e a API
 * da máquina de estados.
 *
 * Camada: Kernel
 *
 *****************************************************************************/

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

/*===========================================================================
 * Enumeração de estados
 *===========================================================================*/
typedef enum {
    ST_INIT = 0,

    /* Menu principal (cíclico) */
    ST_MENU_CONFIG,
    ST_MENU_ADC,
    ST_MENU_CONTROL,

    /* Sub-menus de configuração (cíclico) */
    ST_CFG_CLOCK,
    ST_CFG_LIMITS,
    ST_CFG_LANG,

    /* Estados de edição */
    ST_EDIT_CLOCK,
    ST_EDIT_LIMITS,
    ST_EDIT_LANG,

    /* Sub-estados de visualização */
    ST_SUB_ADC_VIEW,
    ST_SUB_CTRL_VIEW,

    /* Total de estados */
    STATE_COUNT
} state_t;

/*===========================================================================
 * Enumeração de eventos
 *===========================================================================*/
typedef enum {
    EVT_NONE = 0,

    /* Eventos de entrada física */
    EVT_ENC_CW,            /* Encoder sentido horário              */
    EVT_ENC_CCW,           /* Encoder sentido anti-horário         */
    EVT_ENC_SW,            /* Botão do encoder pressionado         */
    EVT_KEY1,              /* Tecla 1 (Voltar / Confirmar)         */

    /* Eventos de comando serial */
    EVT_SERIAL_FWD,        /* Comando 'f' (forward menu)           */
    EVT_SERIAL_STATUS,     /* Comando 'a' (status ADC)             */
    EVT_SERIAL_SET_HI,     /* Comando 'Hxxxx' (setar limite Hi)    */
    EVT_SERIAL_SET_LO,     /* Comando 'Lxxxx' (setar limite Lo)    */
    EVT_SERIAL_SET_LANG,   /* Comando 'Ix'    (setar idioma)       */

    /* Eventos de sistema */
    EVT_TICK,              /* Tick periódico (SysTick ~10ms)       */
    EVT_ALARM_TRIGGER,     /* ADC fora dos limites                 */
    EVT_ALARM_CLEAR,       /* ADC voltou para faixa normal         */

    /* Total de eventos */
    EVENT_COUNT
} event_t;

/*===========================================================================
 * Enumeração de saídas / ações
 *===========================================================================*/
typedef enum {
    OUT_NONE = 0,
    OUT_LCD_UPDATE,        /* Atualizar display LCD                */
    OUT_ALARM_ON,          /* Ativar alarme (buzzer + LED)         */
    OUT_ALARM_OFF,         /* Desativar alarme                     */
    OUT_SAVE_VARS,         /* Salvar variáveis na EEPROM           */
    OUT_SEND_STATUS,       /* Enviar status via serial             */
    OUTPUT_COUNT
} output_t;

/*===========================================================================
 * Estrutura de resultado de transição
 *===========================================================================*/
typedef struct {
    state_t  nextState;    /* Próximo estado                       */
    output_t action;       /* Ação de saída a executar             */
} transition_t;

/*===========================================================================
 * Tipo de handler de estado
 * Cada estado possui uma função handler que recebe o evento e retorna
 * a transição (próximo estado + ação).
 *===========================================================================*/
typedef transition_t (*StateHandler)(event_t evt);

/*===========================================================================
 * Variável global para valor serial recebido
 * Usada para transportar o valor numérico dos comandos Hxxxx, Lxxxx, Ix
 *===========================================================================*/
extern volatile uint16_t serial_param_value;

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Inicializa a máquina de estados.
 *         Carrega o último estado salvo da EEPROM (via módulo var).
 *         Se não houver dados válidos, inicia em ST_MENU_CONFIG.
 */
void ESM_Init(void);

/**
 * @brief  Processa um evento na máquina de estados.
 *         Despacha para o handler do estado atual e executa a transição.
 * @param  evt  Evento a ser processado.
 * @return Ação de saída resultante da transição.
 */
output_t ESM_ProcessEvent(event_t evt);

/**
 * @brief  Retorna o estado atual da máquina.
 * @return Estado atual (state_t).
 */
state_t ESM_GetState(void);

/**
 * @brief  Atualiza o valor ADC cacheado internamente.
 *         Deve ser chamada pelo main loop antes de processar eventos.
 * @param  adcVal  Valor ADC lido (0–1023).
 */
void ESM_UpdateADC(uint16_t adcVal);

/**
 * @brief  Retorna o último valor ADC cacheado.
 * @return Valor ADC (0–1023).
 */
uint16_t ESM_GetCachedADC(void);

/**
 * @brief  Incrementa o cronômetro interno.
 *         Deve ser chamada pelo SysTick handler a cada 10ms.
 */
void ESM_TickChrono(void);

/**
 * @brief  Retorna o tempo do cronômetro em segundos.
 * @return Segundos decorridos.
 */
uint32_t ESM_GetChronoSeconds(void);

/**
 * @brief  Atualiza o display LCD conforme o estado atual da ESM.
 *         Deve ser chamada pelo main loop quando ESM_ProcessEvent
 *         retornar OUT_LCD_UPDATE.
 */
void ESM_UpdateDisplay(void);

#endif /* STATE_MACHINE_H */
