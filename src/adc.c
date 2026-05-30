#include "adc.h"
#include "LPC11Uxx.h"
#include "bits.h"

void adcInit(void) {
    // Habilita o clock para o bloco IOCON e ADC
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16); // IOCON (UM10398, 3.5.14)
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 13); // ADC (UM10398, 3.5.14)

    bitClr(LPC_SYSCON->PDRUNCFG, 4);   // ativar o AD (UM10398,3.5.36)

    // configurar o pino PIO0_12 (AD1) como entrada analógica (UM10398, 7.4.30)
    // FUNC=2, MODE=0 e ADMODE=0
    LPC_IOCON->TMS_PIO0_12 = 0x2;

    // selecionar o canal AD1 e configurar seu clock para 4 MHz (UM10398, 25.5.1)
    // SEL = canal AD1, // CLKDIV = 11 → 48 MHz / 12 = 4 MHz
    LPC_ADC->CR = (1 << 1) | (11 << 8);      
}

unsigned int adcRead(void){
    LPC_ADC->CR |= (1 << 24); // inicia conversão (bit START)

    while ((LPC_ADC->DR[1] & (1 << 31)) == 0); // espera fim (bit DONE)

    return (LPC_ADC->DR[1] >> 6) & 0x3FF; // extrai os 10 bits (resultado)
}
