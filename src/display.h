/*****************************************************************************
 * display.h
 *
 * Módulo de IHM (Interface Homem-Máquina) — gerencia as telas do LCD 16x2
 * com suporte bilíngue (PT/EN).
 *
 * Camada: Aplicação
 *
 *****************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "stateMachine.h"

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Exibe a tela do menu principal correspondente ao estado.
 * @param  st   Estado atual (ST_MENU_CONFIG, ST_MENU_ADC, ST_MENU_CONTROL).
 * @param  lang Idioma (LANG_PT ou LANG_EN).
 */
void DISP_ShowMenu(state_t st, uint8_t lang);

/**
 * @brief  Exibe a tela de sub-menu de configuração.
 * @param  st   Estado atual (ST_CFG_CLOCK, ST_CFG_LIMITS, ST_CFG_LANG).
 * @param  lang Idioma.
 */
void DISP_ShowSubConfig(state_t st, uint8_t lang);

/**
 * @brief  Exibe a tela de visualização do ADC em tempo real.
 * @param  adcVal  Valor atual do ADC (0–1023).
 * @param  lang    Idioma.
 */
void DISP_ShowADC(uint16_t adcVal, uint8_t lang);

/**
 * @brief  Exibe a tela de edição de limites Hi/Lo do alarme.
 * @param  hi      Valor atual do limite superior.
 * @param  lo      Valor atual do limite inferior.
 * @param  cursor  0 = editando Hi, 1 = editando Lo.
 * @param  lang    Idioma.
 */
void DISP_ShowEditLimits(uint16_t hi, uint16_t lo, uint8_t cursor, uint8_t lang);

/**
 * @brief  Exibe a tela de seleção de idioma.
 * @param  lang  Idioma selecionado atualmente.
 */
void DISP_ShowEditLang(uint8_t lang);

/**
 * @brief  Exibe a tela do cronômetro.
 * @param  elapsedSec  Tempo decorrido em segundos.
 * @param  lang        Idioma.
 */
void DISP_ShowClock(uint32_t elapsedSec, uint8_t lang);

/**
 * @brief  Exibe a tela de controle (status do alarme).
 * @param  alarmOn  1 se alarme ativo, 0 caso contrário.
 * @param  lang     Idioma.
 */
void DISP_ShowControl(uint8_t alarmOn, uint8_t lang);

/**
 * @brief  Exibe a tela de alerta de alarme (sobreposição de emergência).
 * @param  adcVal   Valor ADC que disparou o alarme.
 * @param  hiOrLo   1 = acima do Hi, 2 = abaixo do Lo.
 * @param  lang     Idioma.
 */
void DISP_ShowAlarmAlert(uint16_t adcVal, uint8_t hiOrLo, uint8_t lang);

#endif /* DISPLAY_H */
