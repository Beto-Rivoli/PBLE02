/*****************************************************************************
 * var.h
 *
 * Módulo de variáveis persistentes e gerenciamento de EEPROM.
 * Armazena e recupera: idioma, limites de alarme, estado, logs de eventos.
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#ifndef VAR_H
#define VAR_H

#include <stdint.h>

/*===========================================================================
 * Constantes de idioma
 *===========================================================================*/
#define LANG_PT     0
#define LANG_EN     1

/*===========================================================================
 * Constantes de tipo de alarme (para log)
 *===========================================================================*/
#define ALARM_TYPE_HI   1   /* ADC acima do limite superior */
#define ALARM_TYPE_LO   2   /* ADC abaixo do limite inferior */

/*===========================================================================
 * Valores padrão
 *===========================================================================*/
#define DEFAULT_ALARM_HI    800
#define DEFAULT_ALARM_LO    200
#define DEFAULT_LANGUAGE    LANG_PT

/*===========================================================================
 * Estrutura de log de evento de alarme
 *===========================================================================*/
typedef struct {
    uint32_t timestamp;     /* Tick do cronômetro (em segundos)     */
    uint16_t adcValue;      /* Valor ADC no momento do evento       */
    uint8_t  alarmType;     /* ALARM_TYPE_HI ou ALARM_TYPE_LO       */
    uint8_t  reserved;      /* Padding para alinhamento (8 bytes)   */
} alarmLog_t;

/*===========================================================================
 * Estrutura principal de variáveis do sistema
 *===========================================================================*/
typedef struct {
    uint8_t   language;     /* LANG_PT ou LANG_EN                   */
    uint16_t  alarmHi;      /* Limite superior do alarme (0–1023)   */
    uint16_t  alarmLo;      /* Limite inferior do alarme (0–1023)   */
    uint8_t   lastState;    /* Último estado salvo (state_t)        */
    uint8_t   logCount;     /* Número de logs válidos (0–3)         */
    alarmLog_t logs[3];     /* Buffer circular dos 3 últimos eventos*/
} sysVars_t;

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Inicializa o módulo de variáveis.
 *         Lê o magic byte da EEPROM. Se válido (0xA5), carrega todos os
 *         dados persistidos. Caso contrário, inicializa com valores padrão
 *         e grava na EEPROM.
 */
void VAR_Init(void);

/**
 * @brief  Salva todas as variáveis do sistema na EEPROM.
 *         Usa escrita por página para eficiência.
 */
void VAR_Save(void);

/**
 * @brief  Salvamento emergencial (para uso na ISR do BOD).
 *         Salva apenas os dados mais críticos byte a byte,
 *         priorizando velocidade sobre completude.
 */
void VAR_SaveEmergency(void);

/**
 * @brief  Retorna o idioma atual.
 * @return LANG_PT ou LANG_EN.
 */
uint8_t VAR_GetLanguage(void);

/**
 * @brief  Define o idioma.
 * @param  lang  LANG_PT ou LANG_EN.
 */
void VAR_SetLanguage(uint8_t lang);

/**
 * @brief  Retorna o limite superior do alarme.
 * @return Valor ADC (0–1023).
 */
uint16_t VAR_GetAlarmHi(void);

/**
 * @brief  Define o limite superior do alarme.
 * @param  val  Valor ADC (0–1023).
 */
void VAR_SetAlarmHi(uint16_t val);

/**
 * @brief  Retorna o limite inferior do alarme.
 * @return Valor ADC (0–1023).
 */
uint16_t VAR_GetAlarmLo(void);

/**
 * @brief  Define o limite inferior do alarme.
 * @param  val  Valor ADC (0–1023).
 */
void VAR_SetAlarmLo(uint16_t val);

/**
 * @brief  Retorna o último estado salvo.
 * @return state_t codificado como uint8_t.
 */
uint8_t VAR_GetLastState(void);

/**
 * @brief  Define o último estado salvo.
 * @param  st  Estado (state_t codificado como uint8_t).
 */
void VAR_SetLastState(uint8_t st);

/**
 * @brief  Registra um evento de alarme no log circular.
 *         Armazena os 3 mais recentes, descartando o mais antigo.
 * @param  log  Estrutura com dados do evento.
 */
void VAR_LogEvent(alarmLog_t log);

/**
 * @brief  Recupera um log de evento de alarme.
 * @param  index  Índice do log (0 = mais recente, 2 = mais antigo).
 * @return Estrutura do log. Se índice inválido, retorna log com zeros.
 */
alarmLog_t VAR_GetLog(uint8_t index);

/**
 * @brief  Retorna o número de logs válidos armazenados.
 * @return 0 a 3.
 */
uint8_t VAR_GetLogCount(void);

/**
 * @brief  Acesso direto à estrutura de variáveis (somente leitura).
 * @return Ponteiro const para sysVars_t.
 */
const sysVars_t* VAR_GetAll(void);

#endif /* VAR_H */
