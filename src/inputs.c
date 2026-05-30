/*****************************************************************************
 * inputs.c
 *
 * Driver para leitura de teclado (3 teclas) e encoder rotativo com clique
 * no microcontrolador LPC11U14 (LQFP48).
 *
 * Teclado (Ativo-Alto, pull-down externo):
 *   Tecla1 = PIO1_25, Tecla2 = PIO1_26, Tecla3 = PIO1_27
 *
 * Encoder Rotativo (Ativo-Baixo, pull-up externo + RC debounce):
 *   Canal A = PIO1_13, Canal B = PIO1_14, Botao SW = PIO1_15
 *
 * A leitura do encoder utiliza interrupcao de borda de descida no canal A
 * (FLEX_INT0). Na borda de descida de A, o canal B e amostrado:
 *   - B == 0 -> sentido horario   -> encoder_counter++
 *   - B == 1 -> sentido anti-horario -> encoder_counter--
 *
 *****************************************************************************/

#include "LPC11Uxx.h"
#include "inputs.h"

/*===========================================================================
 * Variavel global do contador do encoder
 *===========================================================================*/
volatile int32_t encoder_counter = 0;

/*===========================================================================
 * Estado anterior do canal A para a funcao de polling
 *===========================================================================*/
static uint8_t enc_a_last = 1;  /* Repouso = alto (pull-up externo) */

/*===========================================================================
 *  Mascaras de IOCON
 *
 *  Bits do registrador IOCON para pinos PIO1_13..PIO1_27 (tipo D):
 *    [2:0]  FUNC   - Funcao do pino (0 = GPIO)
 *    [4:3]  MODE   - Pull-up/pull-down interno
 *                     00 = Inativo (sem pull)
 *                     01 = Pull-down
 *                     10 = Pull-up
 *                     11 = Repeater
 *    [5]    HYS    - Histerese (0 = desativada, 1 = ativada)
 *    [6]    INV    - Inversao de entrada (0 = nao invertido)
 *    [9:7]  reservado
 *    [10]   OD     - Open-drain (0 = desativado)
 *===========================================================================*/

/* Teclado: FUNC=0 (GPIO), MODE=01 (pull-down), HYS=0, INV=0, OD=0
 *   -> Valor: (0 << 0) | (1 << 3) = 0x08
 *   Pull-down interno ativado para nao conflitar com pull-down externo.
 *   Garantimos que o pull-up interno esta DESATIVADO.                      */
#define IOCON_KEY_CFG   (0x00 | (1 << 3))   /* 0x08 */

/* Encoder: FUNC=0 (GPIO), MODE=10 (pull-up), HYS=0, INV=0, OD=0
 *   -> Valor: (0 << 0) | (2 << 3) = 0x10
 *   Pull-up interno ativado, compativel com pull-up externo.               */
#define IOCON_ENC_CFG   (0x00 | (2 << 3))   /* 0x10 */

/*===========================================================================
 *  init_inputs()
 *
 *  1. Habilita clock do bloco IOCON (bit 16 do SYSAHBCLKCTRL)
 *  2. Configura IOCON para cada pino
 *  3. Configura pinos como entrada (limpa bits em DIR[1])
 *  4. Configura interrupcao de borda de descida no canal A (FLEX_INT0)
 *===========================================================================*/
void init_inputs(void)
{
    /*-----------------------------------------------------------------------
     *  1. Habilitar clocks necessarios no SYSAHBCLKCTRL:
     *     Bit 6  = GPIO           (normalmente ja habilitado apos reset)
     *     Bit 16 = IOCON          (configuracao de pinos)
     *     Bit 19 = PINT           (GPIO Pin Interrupt - necessario para
     *                              o FLEX_INT0 do encoder funcionar!)
     *-----------------------------------------------------------------------*/
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 6) | (1 << 16) | (1 << 19);

    /*-----------------------------------------------------------------------
     *  2. Configurar registradores IOCON para cada pino
     *-----------------------------------------------------------------------*/

    /* --- Teclado (ativo-alto, pull-down) --- */
    LPC_IOCON->PIO1_25 = IOCON_KEY_CFG;   /* Tecla 1 */
    LPC_IOCON->PIO1_26 = IOCON_KEY_CFG;   /* Tecla 2 */
    LPC_IOCON->PIO1_27 = IOCON_KEY_CFG;   /* Tecla 3 */

    /* --- Encoder rotativo (ativo-baixo, pull-up) --- */
    LPC_IOCON->PIO1_13 = IOCON_ENC_CFG;   /* Canal A */
    LPC_IOCON->PIO1_14 = IOCON_ENC_CFG;   /* Canal B */
    LPC_IOCON->PIO1_15 = IOCON_ENC_CFG;   /* Botao SW */

    /*-----------------------------------------------------------------------
     *  3. Configurar pinos como ENTRADA na Porta 1
     *     Limpar os bits correspondentes em LPC_GPIO->DIR[1]
     *     (bit = 0 -> entrada, bit = 1 -> saida)
     *-----------------------------------------------------------------------*/
    LPC_GPIO->DIR[1] &= ~( (1 << KEY1_PIN)   |
                            (1 << KEY2_PIN)   |
                            (1 << KEY3_PIN)   |
                            (1 << ENC_A_PIN)  |
                            (1 << ENC_B_PIN)  |
                            (1 << ENC_SW_PIN) );

    /*-----------------------------------------------------------------------
     *  4. Configurar interrupcao de borda de descida no canal A do encoder
     *     usando o modulo GPIO Pin Interrupt (FLEX_INT0)
     *
     *     Passos:
     *     a) Selecionar o pino PIO1_13 para o canal PINTSEL[0]
     *        PINTSEL: bits [4:0] = numero do pino, bit [5] = porta
     *        Para PIO1_13: (1 * 24) + 13 = 37 = 0x25
     *        (Os pinos da porta 1 comecam no offset 24)
     *     b) Configurar ISEL  = 0 (borda, nao nivel)
     *     c) Habilitar borda de descida (IENF) para o canal 0
     *     d) Limpar qualquer flag pendente
     *     e) Habilitar NVIC para FLEX_INT0_IRQn
     *-----------------------------------------------------------------------*/

    /* a) Mapear PIO1_13 -> PINTSEL[0]
     *    Porta 1 pino 13 -> indice = 24 + 13 = 37                         */
    LPC_SYSCON->PINTSEL[0] = (24 + ENC_A_PIN);   /* 24 + 13 = 37 */

    /* b) Modo de borda (nao nivel) para canal 0 */
    LPC_GPIO_PIN_INT->ISEL &= ~(1 << 0);

    /* c) Desabilitar borda de subida, habilitar borda de descida */
    LPC_GPIO_PIN_INT->CIENR = (1 << 0);   /* Limpa habilitacao de rising  */
    LPC_GPIO_PIN_INT->SIENF = (1 << 0);   /* Seta habilitacao de falling  */

    /* d) Limpar flags pendentes */
    LPC_GPIO_PIN_INT->IST = (1 << 0);

    /* e) Habilitar interrupcao no NVIC */
    NVIC_EnableIRQ(FLEX_INT0_IRQn);

    /* Inicializar estado anterior do encoder */
    enc_a_last = (LPC_GPIO->PIN[1] >> ENC_A_PIN) & 1;
}

/*===========================================================================
 *  Funcoes de leitura do teclado (polling)
 *  Logica: Ativo-Alto -> pino em nivel alto = tecla pressionada
 *===========================================================================*/

int key1_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY1_PIN)) ? 1 : 0;
}

int key2_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY2_PIN)) ? 1 : 0;
}

int key3_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << KEY3_PIN)) ? 1 : 0;
}

/*===========================================================================
 *  Funcao de leitura do botao do encoder (polling)
 *  Logica: Ativo-Baixo -> pino em nivel baixo = botao pressionado
 *===========================================================================*/

int enc_button_pressed(void)
{
    return (LPC_GPIO->PIN[1] & (1 << ENC_SW_PIN)) ? 0 : 1;
}

/*===========================================================================
 *  ISR do encoder (interrupcao de borda de descida no canal A)
 *
 *  Quando ocorre borda de descida em A (transicao 1->0):
 *    - Se B == 0 -> sentido horario   -> encoder_counter++
 *    - Se B == 1 -> sentido anti-horario -> encoder_counter--
 *
 *  O nome FLEX_INT0_IRQHandler corresponde ao vetor FLEX_INT0 no startup,
 *  declarado como WEAK, portanto esta definicao sobrescreve o handler padrao.
 *===========================================================================*/

void FLEX_INT0_IRQHandler(void)
{
    /* Verifica se a interrupcao e de borda de descida (falling) */
    if (LPC_GPIO_PIN_INT->FALL & (1 << 0))
    {
        /* Ler o estado atual do canal B */
        uint32_t b_state = (LPC_GPIO->PIN[1] >> ENC_B_PIN) & 1;

        if (b_state == 0)
        {
            /* Canal B baixo na borda de descida de A -> horario */
            encoder_counter++;
        }
        else
        {
            /* Canal B alto na borda de descida de A -> anti-horario */
            encoder_counter--;
        }

        /* Limpar a flag de borda de descida escrevendo 1 no bit */
        LPC_GPIO_PIN_INT->FALL = (1 << 0);
    }

    /* Limpar flag de status da interrupcao */
    LPC_GPIO_PIN_INT->IST = (1 << 0);
}

/*===========================================================================
 *  Funcao de polling do encoder (alternativa a interrupcao)
 *
 *  Detecta transicao de borda de descida no canal A por comparacao de
 *  estado anterior vs. atual. Util quando nao se deseja usar interrupcao.
 *
 *  NOTA: Se a interrupcao FLEX_INT0 estiver ativa, NAO chame esta funcao,
 *  pois ambas atualizariam o encoder_counter de forma duplicada.
 *===========================================================================*/

void encoder_poll(void)
{
    uint8_t a_now = (LPC_GPIO->PIN[1] >> ENC_A_PIN) & 1;

    /* Detectar borda de descida: anterior = 1, atual = 0 */
    if (enc_a_last == 1 && a_now == 0)
    {
        uint8_t b_now = (LPC_GPIO->PIN[1] >> ENC_B_PIN) & 1;

        if (b_now == 0)
        {
            encoder_counter++;   /* Sentido horario */
        }
        else
        {
            encoder_counter--;   /* Sentido anti-horario */
        }
    }

    enc_a_last = a_now;
}

/*===========================================================================
 *  Geração de eventos com debounce baseado em timer
 *
 *  inputs_tick() é chamada pelo SysTick a cada 10ms.
 *  Amostra o estado das teclas e encoder SW, requerendo 3 leituras
 *  consecutivas iguais (30ms de debounce) para confirmar uma pressão.
 *
 *  inputs_getEvent() retorna o evento de maior prioridade pendente.
 *===========================================================================*/

/* Contadores de debounce (3 = confirmado) */
#define DEBOUNCE_COUNT  3

static uint8_t key1_deb = 0;
static uint8_t enc_sw_deb = 0;

/* Flags de evento (setadas pelo debounce, limpas pela leitura) */
static volatile uint8_t flag_key1   = 0;
static volatile uint8_t flag_enc_sw = 0;

/* Snapshot do encoder_counter para detecção de delta */
static int32_t enc_last_snapshot = 0;

void inputs_tick(void)
{
    /*--- Debounce da Tecla 1 (ativo-alto) ---*/
    if (key1_pressed()) {
        if (key1_deb < DEBOUNCE_COUNT) {
            key1_deb++;
            if (key1_deb == DEBOUNCE_COUNT) {
                flag_key1 = 1;
            }
        }
    } else {
        key1_deb = 0;
    }

    /*--- Debounce do botão do encoder (ativo-baixo) ---*/
    if (enc_button_pressed()) {
        if (enc_sw_deb < DEBOUNCE_COUNT) {
            enc_sw_deb++;
            if (enc_sw_deb == DEBOUNCE_COUNT) {
                flag_enc_sw = 1;
            }
        }
    } else {
        enc_sw_deb = 0;
    }
}

event_t inputs_getEvent(void)
{
    /*--- Prioridade 1: Rotação do encoder (via ISR) ---*/
    int32_t current = encoder_counter;
    int32_t delta = current - enc_last_snapshot;

    if (delta > 0) {
        enc_last_snapshot = current;
        return EVT_ENC_CW;
    }
    if (delta < 0) {
        enc_last_snapshot = current;
        return EVT_ENC_CCW;
    }

    /*--- Prioridade 2: Botão do encoder ---*/
    if (flag_enc_sw) {
        flag_enc_sw = 0;
        return EVT_ENC_SW;
    }

    /*--- Prioridade 3: Tecla 1 ---*/
    if (flag_key1) {
        flag_key1 = 0;
        return EVT_KEY1;
    }

    return EVT_NONE;
}

