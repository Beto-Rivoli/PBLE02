/*****************************************************************************
 * inputs.h
 *
 * Driver para leitura de teclado (3 teclas) e encoder rotativo com clique
 * no microcontrolador LPC11U14 (LQFP48).
 *
 * Mapeamento de Hardware (tudo na Porta 1):
 *   Teclado:  Tecla1 = PIO1_25, Tecla2 = PIO1_26, Tecla3 = PIO1_27
 *             Logica Ativo-Alto (pull-down externo no hardware)
 *   Encoder:  Canal A = PIO1_13, Canal B = PIO1_14, SW = PIO1_15
 *             Logica Ativo-Baixo (pull-up externo + RC debounce no hardware)
 *
 *****************************************************************************/

#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>

/*===========================================================================
 * Definicoes de pinos (numero do bit na Porta 1)
 *===========================================================================*/

/* Teclado - Porta 1 */
#define KEY1_PIN    25   /* PIO1_25 */
#define KEY2_PIN    26   /* PIO1_26 */
#define KEY3_PIN    27   /* PIO1_27 */

/* Encoder Rotativo - Porta 1 */
#define ENC_A_PIN   13   /* PIO1_13 - Canal A */
#define ENC_B_PIN   14   /* PIO1_14 - Canal B */
#define ENC_SW_PIN  15   /* PIO1_15 - Botao (push) */

/*===========================================================================
 * Variavel global do contador do encoder
 *===========================================================================*/
extern volatile int32_t encoder_counter;

/*===========================================================================
 * Prototipos de funcoes
 *===========================================================================*/

/**
 * @brief Inicializa todos os pinos de entrada (teclado + encoder).
 *        Configura clock do IOCON, registradores IOCON e direcao GPIO.
 *        Configura interrupção de borda de descida no canal A do encoder
 *        (FLEX_INT0) via GPIO_PIN_INT.
 */
void init_inputs(void);

/**
 * @brief Verifica se a Tecla 1 esta pressionada (ativo-alto).
 * @return 1 se pressionada, 0 caso contrario.
 */
int key1_pressed(void);

/**
 * @brief Verifica se a Tecla 2 esta pressionada (ativo-alto).
 * @return 1 se pressionada, 0 caso contrario.
 */
int key2_pressed(void);

/**
 * @brief Verifica se a Tecla 3 esta pressionada (ativo-alto).
 * @return 1 se pressionada, 0 caso contrario.
 */
int key3_pressed(void);

/**
 * @brief Verifica se o botao do encoder esta pressionado (ativo-baixo).
 * @return 1 se pressionado, 0 caso contrario.
 */
int enc_button_pressed(void);

/**
 * @brief Atualiza o encoder por polling (alternativa a interrupcao).
 *        Chame periodicamente no loop principal se nao usar interrupcao.
 *        Quando a interrupcao esta ativa, esta funcao nao precisa ser chamada.
 */
void encoder_poll(void);

/*===========================================================================
 * Geração de eventos com debounce baseado em timer
 * (requer chamada periódica de inputs_tick() pelo SysTick)
 *===========================================================================*/

#include "stateMachine.h"

/**
 * @brief  Deve ser chamada pelo SysTick handler a cada 10ms.
 *         Atualiza contadores de debounce das teclas e encoder SW.
 */
void inputs_tick(void);

/**
 * @brief  Retorna o próximo evento de entrada pendente.
 *         Verifica flags atômicas de encoder (delta) e teclas (debounced).
 *         Consome o evento (flag é limpa após leitura).
 * @return event_t correspondente, ou EVT_NONE se não houver evento.
 */
event_t inputs_getEvent(void);

#endif /* INPUTS_H */
