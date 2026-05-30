/*****************************************************************************
 * var.c
 *
 * Módulo de variáveis persistentes — implementação.
 * Gerencia leitura/escrita de configurações e logs na EEPROM 24LC512.
 *
 * Mapa de Endereços EEPROM:
 *   0x0100 — Magic byte (0xA5 = dados válidos)
 *   0x0101 — Idioma (1 byte)
 *   0x0102 — Alarme Hi MSB, 0x0103 — Alarme Hi LSB
 *   0x0104 — Alarme Lo MSB, 0x0105 — Alarme Lo LSB
 *   0x0106 — Último estado salvo (1 byte)
 *   0x0107 — Número de logs (1 byte)
 *   0x0110..0x0117 — Log evento 0 (8 bytes)
 *   0x0118..0x011F — Log evento 1 (8 bytes)
 *   0x0120..0x0127 — Log evento 2 (8 bytes)
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#include "var.h"
#include "eeprom_24lc512.h"

/*===========================================================================
 * Endereços na EEPROM
 *===========================================================================*/
#define EEPROM_MAGIC_ADDR       0x0100
#define EEPROM_MAGIC_VALUE      0xA5

#define EEPROM_LANG_ADDR        0x0101
#define EEPROM_HI_MSB_ADDR      0x0102
#define EEPROM_HI_LSB_ADDR      0x0103
#define EEPROM_LO_MSB_ADDR      0x0104
#define EEPROM_LO_LSB_ADDR      0x0105
#define EEPROM_STATE_ADDR       0x0106
#define EEPROM_LOGCOUNT_ADDR    0x0107

#define EEPROM_LOG0_ADDR        0x0110
#define EEPROM_LOG1_ADDR        0x0118
#define EEPROM_LOG2_ADDR        0x0120

#define LOG_ENTRY_SIZE          8   /* sizeof(alarmLog_t) = 8 bytes */

/*===========================================================================
 * Variável estática do sistema (cópia em RAM)
 *===========================================================================*/
static sysVars_t sysVars;

/*===========================================================================
 * Funções internas
 *===========================================================================*/

/**
 * @brief Serializa e grava um log entry na EEPROM.
 */
static void writeLogEntry(uint16_t baseAddr, const alarmLog_t *log)
{
    uint8_t buf[LOG_ENTRY_SIZE];
    buf[0] = (uint8_t)(log->timestamp >> 24);
    buf[1] = (uint8_t)(log->timestamp >> 16);
    buf[2] = (uint8_t)(log->timestamp >> 8);
    buf[3] = (uint8_t)(log->timestamp & 0xFF);
    buf[4] = (uint8_t)(log->adcValue >> 8);
    buf[5] = (uint8_t)(log->adcValue & 0xFF);
    buf[6] = log->alarmType;
    buf[7] = 0x00;  /* reservado */

    EEPROM_WritePage(baseAddr, buf, LOG_ENTRY_SIZE);
}

/**
 * @brief Lê um log entry da EEPROM e deserializa.
 */
static void readLogEntry(uint16_t baseAddr, alarmLog_t *log)
{
    uint8_t buf[LOG_ENTRY_SIZE];
    EEPROM_ReadSequential(baseAddr, buf, LOG_ENTRY_SIZE);

    log->timestamp = ((uint32_t)buf[0] << 24) |
                     ((uint32_t)buf[1] << 16) |
                     ((uint32_t)buf[2] << 8)  |
                     ((uint32_t)buf[3]);
    log->adcValue  = ((uint16_t)buf[4] << 8) | buf[5];
    log->alarmType = buf[6];
    log->reserved  = 0;
}

/**
 * @brief Retorna o endereço base do log pelo índice (0–2).
 */
static uint16_t logAddr(uint8_t index)
{
    switch (index) {
        case 0:  return EEPROM_LOG0_ADDR;
        case 1:  return EEPROM_LOG1_ADDR;
        case 2:  return EEPROM_LOG2_ADDR;
        default: return EEPROM_LOG0_ADDR;
    }
}

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void VAR_Init(void)
{
    uint8_t magic = 0x00;
    EEPROM_ReadByte(EEPROM_MAGIC_ADDR, &magic);

    if (magic == EEPROM_MAGIC_VALUE) {
        /* Dados válidos — carregar da EEPROM */
        uint8_t hi_msb, hi_lsb, lo_msb, lo_lsb;

        EEPROM_ReadByte(EEPROM_LANG_ADDR, &sysVars.language);
        EEPROM_ReadByte(EEPROM_HI_MSB_ADDR, &hi_msb);
        EEPROM_ReadByte(EEPROM_HI_LSB_ADDR, &hi_lsb);
        EEPROM_ReadByte(EEPROM_LO_MSB_ADDR, &lo_msb);
        EEPROM_ReadByte(EEPROM_LO_LSB_ADDR, &lo_lsb);
        EEPROM_ReadByte(EEPROM_STATE_ADDR, &sysVars.lastState);
        EEPROM_ReadByte(EEPROM_LOGCOUNT_ADDR, &sysVars.logCount);

        sysVars.alarmHi = ((uint16_t)hi_msb << 8) | hi_lsb;
        sysVars.alarmLo = ((uint16_t)lo_msb << 8) | lo_lsb;

        /* Validação de limites */
        if (sysVars.alarmHi > 1023) sysVars.alarmHi = 1023;
        if (sysVars.alarmLo > 1023) sysVars.alarmLo = 0;
        if (sysVars.language > LANG_EN) sysVars.language = LANG_PT;
        if (sysVars.logCount > 3) sysVars.logCount = 0;

        /* Carregar logs */
        uint8_t i;
        for (i = 0; i < sysVars.logCount && i < 3; i++) {
            readLogEntry(logAddr(i), &sysVars.logs[i]);
        }
    } else {
        /* EEPROM virgem — inicializar com defaults */
        sysVars.language  = DEFAULT_LANGUAGE;
        sysVars.alarmHi   = DEFAULT_ALARM_HI;
        sysVars.alarmLo   = DEFAULT_ALARM_LO;
        sysVars.lastState = 1;  /* ST_MENU_CONFIG */
        sysVars.logCount  = 0;

        uint8_t i;
        for (i = 0; i < 3; i++) {
            sysVars.logs[i].timestamp = 0;
            sysVars.logs[i].adcValue  = 0;
            sysVars.logs[i].alarmType = 0;
            sysVars.logs[i].reserved  = 0;
        }

        /* Gravar defaults na EEPROM */
        VAR_Save();
    }
}

void VAR_Save(void)
{
    /* Gravar magic byte */
    EEPROM_WriteByte(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);

    /* Gravar variáveis individuais */
    EEPROM_WriteByte(EEPROM_LANG_ADDR, sysVars.language);
    EEPROM_WriteByte(EEPROM_HI_MSB_ADDR, (uint8_t)(sysVars.alarmHi >> 8));
    EEPROM_WriteByte(EEPROM_HI_LSB_ADDR, (uint8_t)(sysVars.alarmHi & 0xFF));
    EEPROM_WriteByte(EEPROM_LO_MSB_ADDR, (uint8_t)(sysVars.alarmLo >> 8));
    EEPROM_WriteByte(EEPROM_LO_LSB_ADDR, (uint8_t)(sysVars.alarmLo & 0xFF));
    EEPROM_WriteByte(EEPROM_STATE_ADDR, sysVars.lastState);
    EEPROM_WriteByte(EEPROM_LOGCOUNT_ADDR, sysVars.logCount);

    /* Gravar logs */
    uint8_t i;
    for (i = 0; i < sysVars.logCount && i < 3; i++) {
        writeLogEntry(logAddr(i), &sysVars.logs[i]);
    }
}

void VAR_SaveEmergency(void)
{
    /*
     * Versão minimalista para uso em ISR do BOD.
     * Salva apenas: magic, idioma, limites e estado.
     * Não grava logs (economia de tempo no brown-out).
     */
    EEPROM_WriteByte(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM_WriteByte(EEPROM_LANG_ADDR, sysVars.language);
    EEPROM_WriteByte(EEPROM_HI_MSB_ADDR, (uint8_t)(sysVars.alarmHi >> 8));
    EEPROM_WriteByte(EEPROM_HI_LSB_ADDR, (uint8_t)(sysVars.alarmHi & 0xFF));
    EEPROM_WriteByte(EEPROM_LO_MSB_ADDR, (uint8_t)(sysVars.alarmLo >> 8));
    EEPROM_WriteByte(EEPROM_LO_LSB_ADDR, (uint8_t)(sysVars.alarmLo & 0xFF));
    EEPROM_WriteByte(EEPROM_STATE_ADDR, sysVars.lastState);
}

uint8_t VAR_GetLanguage(void)
{
    return sysVars.language;
}

void VAR_SetLanguage(uint8_t lang)
{
    if (lang <= LANG_EN) {
        sysVars.language = lang;
    }
}

uint16_t VAR_GetAlarmHi(void)
{
    return sysVars.alarmHi;
}

void VAR_SetAlarmHi(uint16_t val)
{
    if (val <= 1023) {
        sysVars.alarmHi = val;
    }
}

uint16_t VAR_GetAlarmLo(void)
{
    return sysVars.alarmLo;
}

void VAR_SetAlarmLo(uint16_t val)
{
    if (val <= 1023) {
        sysVars.alarmLo = val;
    }
}

uint8_t VAR_GetLastState(void)
{
    return sysVars.lastState;
}

void VAR_SetLastState(uint8_t st)
{
    sysVars.lastState = st;
}

void VAR_LogEvent(alarmLog_t log)
{
    /* Deslocar logs: [0] <- [1] <- [2], novo entra em [0] */
    sysVars.logs[2] = sysVars.logs[1];
    sysVars.logs[1] = sysVars.logs[0];
    sysVars.logs[0] = log;

    if (sysVars.logCount < 3) {
        sysVars.logCount++;
    }
}

alarmLog_t VAR_GetLog(uint8_t index)
{
    alarmLog_t empty = {0, 0, 0, 0};
    if (index < sysVars.logCount && index < 3) {
        return sysVars.logs[index];
    }
    return empty;
}

uint8_t VAR_GetLogCount(void)
{
    return sysVars.logCount;
}

const sysVars_t* VAR_GetAll(void)
{
    return &sysVars;
}
