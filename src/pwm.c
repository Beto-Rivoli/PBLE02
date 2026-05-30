#include "pwm.h"
#include "LPC11Uxx.h"
#include "bits.h"

#define PWM_PERIOD 48000 // 48 MHz / 48000 = 1000 Hz (1 kHz)

void pwmInit(void) {
    // 1. Habilita o clock para o bloco IOCON e o Timer CT32B0
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16); // IOCON
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 9);  // CT32B0 (32-bit Timer 0)

    // 2. Configura o pino PIO0_18 para funcao CT32B0_MAT0 (FUNC=2)
    // Sem resistores de pull-up/pull-down (MODE=0) e modo digital
    LPC_IOCON->PIO0_18 = 0x2;

    // 3. Garante que o temporizador esta em modo temporizador (e nao contador)
    LPC_CT32B0->CTCR = 0x0;

    // 4. Prescaler em 0 (timer roda na frequencia maxima do clock periférico = 48 MHz)
    LPC_CT32B0->PR = 0;

    // 5. Configura o periodo do PWM em MR3 (MR3 definirá o fim do ciclo)
    LPC_CT32B0->MR3 = PWM_PERIOD;

    // 6. Configura o temporizador para reiniciar (reset) quando TC = MR3 (bit 10 de MCR)
    LPC_CT32B0->MCR = (1 << 10);

    // 7. Define o ciclo ativo inicial como 0% (MR0 = 0)
    LPC_CT32B0->MR0 = 0;

    // 8. Ativa a funcao de PWM para o canal 0 (MR0 controla o pino MAT0)
    LPC_CT32B0->PWMC = (1 << 0);

    // 9. Inicializa e inicia o temporizador (bit 0 do TCR ativa o contador)
    LPC_CT32B0->TC = 0;
    LPC_CT32B0->PC = 0;
    LPC_CT32B0->TCR = 1;
}

void pwmSetDuty(uint32_t duty) {
    // Limita o duty cycle entre 0 e 1000
    if (duty > 1000) {
        duty = 1000;
    }

    // Calcula o valor de comparacao proporcional em MR0
    // Duty = 0    -> MR0 = 0 (totalmente desligado)
    // Duty = 1000 -> MR0 = PWM_PERIOD (totalmente ligado)
    LPC_CT32B0->MR0 = (duty * PWM_PERIOD) / 1000;
}
