/*****************************************************************************
 * display.c
 *
 * Módulo de IHM — implementação das telas LCD 16x2 com suporte bilíngue.
 * Todas as strings são organizadas em tabelas de lookup indexadas por idioma.
 *
 * LCD 16x2: 2 linhas de 16 caracteres cada.
 * Cada função DISP_Show*() formata as 2 linhas e envia via LCD driver.
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#include "display.h"
#include "lcd.h"
#include "var.h"

/*===========================================================================
 * Tabelas de strings bilíngues [LANG_PT=0][LANG_EN=1]
 *===========================================================================*/

/* Menu principal */
static const char * const str_config[]   = { "CONFIG",   "CONFIG"   };
static const char * const str_adc[]      = { "ADC",      "ADC"      };
static const char * const str_control[]  = { "CONTROLE", "CONTROL"  };

/* Sub-menu configuração */
static const char * const str_cfgtitle[] = { "CONFIGURACAO",   "SETTINGS"      };
static const char * const str_clock[]    = { "> Cronometro",   "> Stopwatch"   };
static const char * const str_limits[]   = { "> Limites Hi/Lo","> Limits Hi/Lo"};
static const char * const str_langopt[]  = { "> Idioma: ",     "> Language: "  };

/* Edição de idioma */
static const char * const str_langtitle[]= { "Idioma/Language","Idioma/Language"};

/* Cronômetro */
static const char * const str_clocktit[] = { "Cronometro",     "Stopwatch"     };

/* Controle */
static const char * const str_ctrltit[]  = { "CONTROLE",       "CONTROL"       };
static const char * const str_alarmon[]  = { "Alarme: LIGADO", "Alarm: ON"     };
static const char * const str_alarmoff[] = { "Alarme: DESL.",  "Alarm: OFF"    };

/* Alerta de alarme */
static const char * const str_alert[]    = { "!! ALARME !!",   "!! ALARM !!"   };

/*===========================================================================
 * Funções auxiliares internas
 *===========================================================================*/

/**
 * @brief Preenche uma linha de 16 caracteres com espaços (limpa resíduo).
 */
static void clearLine(uint8_t row)
{
    LCD_SetCursor(row, 0);
    LCD_String("                ");  /* 16 espaços */
}

/**
 * @brief Imprime um número com N dígitos, preenchido com zeros à esquerda.
 *        Ex: printPadded(42, 4) -> "0042"
 */
static void printPadded(uint16_t val, uint8_t digits)
{
    char buf[6]; /* Máximo 5 dígitos + null */
    int i;

    for (i = digits - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    buf[digits] = '\0';
    LCD_String(buf);
}

/**
 * @brief Imprime "HH:MM:SS" a partir de segundos totais.
 */
static void printTime(uint32_t totalSec)
{
    uint8_t hh = (uint8_t)((totalSec / 3600) % 100);
    uint8_t mm = (uint8_t)((totalSec / 60) % 60);
    uint8_t ss = (uint8_t)(totalSec % 60);

    printPadded(hh, 2);
    LCD_Char(':');
    printPadded(mm, 2);
    LCD_Char(':');
    printPadded(ss, 2);
}

/*===========================================================================
 * Funções públicas
 *===========================================================================*/

void DISP_ShowMenu(state_t st, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    switch (st) {
        case ST_MENU_CONFIG:
            LCD_SetCursor(0, 0);
            LCD_String(">[");
            LCD_String(str_config[lang]);
            LCD_String("]");

            LCD_SetCursor(1, 1);
            LCD_String(str_adc[lang]);
            LCD_String("   ");
            LCD_String(str_control[lang]);
            break;

        case ST_MENU_ADC:
            LCD_SetCursor(0, 1);
            LCD_String(str_config[lang]);

            LCD_SetCursor(1, 0);
            LCD_String(">[");
            LCD_String(str_adc[lang]);
            LCD_String("] ");
            LCD_String("CTRL");
            break;

        case ST_MENU_CONTROL:
            LCD_SetCursor(0, 1);
            LCD_String(str_config[lang]);
            LCD_String("   ");
            LCD_String(str_adc[lang]);

            LCD_SetCursor(1, 0);
            LCD_String(">[");
            LCD_String(str_control[lang]);
            LCD_String("]");
            break;

        default:
            break;
    }
}

void DISP_ShowSubConfig(state_t st, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String(str_cfgtitle[lang]);

    LCD_SetCursor(1, 0);
    switch (st) {
        case ST_CFG_CLOCK:
            LCD_String(str_clock[lang]);
            break;
        case ST_CFG_LIMITS:
            LCD_String(str_limits[lang]);
            break;
        case ST_CFG_LANG:
            LCD_String(str_langopt[lang]);
            LCD_String((lang == LANG_PT) ? "PT" : "EN");
            break;
        default:
            break;
    }
}

void DISP_ShowADC(uint16_t adcVal, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String("ADC: ");
    printPadded(adcVal, 4);

    LCD_SetCursor(1, 0);
    LCD_String("Hi:");
    printPadded(VAR_GetAlarmHi(), 4);
    LCD_String(" Lo:");
    printPadded(VAR_GetAlarmLo(), 4);
}

void DISP_ShowEditLimits(uint16_t hi, uint16_t lo, uint8_t cursor, uint8_t lang)
{
    (void)lang; /* Layout é universal (numérico) */

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    if (cursor == 0) LCD_Char('>');
    else             LCD_Char(' ');
    LCD_String("Hi:");
    printPadded(hi, 4);

    LCD_String(" ");

    if (cursor == 1) LCD_Char('>');
    else             LCD_Char(' ');
    LCD_String("Lo:");
    printPadded(lo, 4);

    LCD_SetCursor(1, 0);
    LCD_String("Enc=Ajust SW=OK ");
}

void DISP_ShowEditLang(uint8_t lang)
{
    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String(str_langtitle[0]); /* Sempre bilíngue no título */

    LCD_SetCursor(1, 0);
    LCD_String("> ");
    if (lang == LANG_PT) {
        LCD_String("[PT] EN");
    } else {
        LCD_String("PT [EN]");
    }
    LCD_String("         ");
}

void DISP_ShowClock(uint32_t elapsedSec, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String(str_clocktit[lang]);

    LCD_SetCursor(1, 1);
    printTime(elapsedSec);
}

void DISP_ShowControl(uint8_t alarmOn, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String(str_ctrltit[lang]);

    LCD_SetCursor(1, 0);
    if (alarmOn) {
        LCD_String(str_alarmon[lang]);
    } else {
        LCD_String(str_alarmoff[lang]);
    }
}

void DISP_ShowAlarmAlert(uint16_t adcVal, uint8_t hiOrLo, uint8_t lang)
{
    if (lang > LANG_EN) lang = LANG_PT;

    clearLine(0);
    clearLine(1);

    LCD_SetCursor(0, 0);
    LCD_String(str_alert[lang]);

    LCD_SetCursor(1, 0);
    LCD_String("ADC:");
    printPadded(adcVal, 4);

    if (hiOrLo == 1) {
        LCD_String(" >Hi");
        printPadded(VAR_GetAlarmHi(), 4);
    } else {
        LCD_String(" <Lo");
        printPadded(VAR_GetAlarmLo(), 4);
    }
}
