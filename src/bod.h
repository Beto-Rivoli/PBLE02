/*****************************************************************************
 * bod.h
 *
 * Módulo de detecção de Brown-Out (BOD) — configura o hardware BOD do
 * LPC11U14 para gerar interrupção em queda de tensão, permitindo
 * salvamento emergencial na EEPROM antes do reset.
 *
 * Registrador BODCTRL (LPC_SYSCON->BODCTRL, offset 0x150):
 *   Bits [1:0] BODRSTLEV  — Nível de reset BOD
 *                  00 = Reserved
 *                  01 = Level 1 (2.06V)
 *                  10 = Level 2 (2.35V)
 *                  11 = Level 3 (2.63V)
 *   Bits [3:2] BODINTVAL  — Nível de interrupção BOD
 *                  00 = Reserved
 *                  01 = Level 1 (2.22V)
 *                  10 = Level 2 (2.52V)
 *                  11 = Level 3 (2.80V)
 *   Bit [4]    BODRSTENA  — Habilita reset por BOD
 *
 * Estratégia:
 *   - Interrupção BOD em 2.80V (nível 3) — detecta queda cedo
 *   - Reset BOD em 2.35V (nível 2) — reset se tensão cair demais
 *   - Na ISR, salva dados críticos na EEPROM (VAR_SaveEmergency)
 *
 * Camada: Driver / Segurança
 *
 *****************************************************************************/

#ifndef BOD_H
#define BOD_H

/*===========================================================================
 * Protótipos de funções públicas
 *===========================================================================*/

/**
 * @brief  Inicializa o detector de Brown-Out (BOD).
 *         Configura níveis de interrupção e reset no BODCTRL.
 *         Habilita a interrupção BOD no NVIC.
 */
void BOD_Init(void);

#endif /* BOD_H */
