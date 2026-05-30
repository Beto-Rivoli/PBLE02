#include "i2c.h"
#include "LPC11Uxx.h"
#include "bits.h"
#include "io.h"
#include "serial.h"

void i2cInit(void){
    // HABILITA CLOCK PARA IOCON E CONFIGURA PINOS
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 16); // Habilitar o clock para o bloco IOCON (UM10398, 3.5.14)
    LPC_IOCON->PIO0_4 = 0x01; // Configurar o pino como SCL em modo I2C padrão (I2CMODE=00) (UM10398, 7.4.11)
    LPC_IOCON->PIO0_5 = 0x01; // Configurar o pino como SDA em modo I2C padrão (I2CMODE=00) (UM10398, 7.4.12)

    // HABILITA CLOCK PARA I2C
    bitSet(LPC_SYSCON->SYSAHBCLKCTRL, 5); // Habilitar o clock para o bloco I2C (UM10398, 3.5.14)
    bitSet(LPC_SYSCON->PRESETCTRL, 1); // Garantir que o periférico I2C não esteja em estado de reset (UM10398, 3.5.2)

    // DEFINE FREQUENCIA 100kHz (para 12MHz de clock)
    LPC_I2C->SCLH = 240; // Estipular o clock I2C para 100kHz (UM10398, 15.7.5.1)
    LPC_I2C->SCLL = 240;

    // HABILITA I2C (mestre síncrono)
    bitSet(LPC_I2C->CONSET, 6); // Habilitar o periférico I2C como mestre (UM10398, 15.7.1 / 15.8.1)
}

void i2cSend(unsigned char endereco, unsigned char *dados, unsigned char qtd){
    unsigned char i;

    LPC_I2C->CONSET = (1 << 5); // Requisitar o evento de START (UM10398, 15.7.1)
    while((LPC_I2C->STAT != 0x08) && (LPC_I2C->STAT != 0x10)); // Esperar pelo pelo fim do evento de START (bit SI, UM10398, 15.7.2)

    LPC_I2C->DAT = endereco << 1; // Enviar o endereço do dispositivo escravo e espeficicação de escrita (R/W=0) (UM10398, 15.7.3)
    LPC_I2C->CONCLR = (1 << 3) | (1 << 5); // Limpar a sinalizações SI e START (UM10398, 15.7.6)
    while(LPC_I2C->STAT != 0x18){ // Esperar por um estado 0x18 (ACK recebido) (UM10398, 15.10.1, tabela 236)
        if(LPC_I2C->STAT == 0x20) return;
    }
    for(i=0; i<qtd; i++){
        LPC_I2C->DAT = dados[i];
        LPC_I2C->CONCLR = (1 << 3); // Limpar a sinalização SI (UM10398, 15.7.6)
        while(LPC_I2C->STAT != 0x28){ // Esperar por um estado 0x28 (ACK recebido) (UM10398, 15.10.1, tabela 236)
            if(LPC_I2C->STAT == 0x30) return; 
        }
    }

    LPC_I2C->CONSET = (1 << 4); // Requisitar o evento de STOP (UM10398, 15.7.1)
    LPC_I2C->CONCLR = (1 << 3); // Limpar a sinalização SI (UM10398, 15.7.6)
}

void i2cReceive(unsigned char endereco, unsigned char *dados, unsigned char qtd){
    unsigned char i;     
    
    LPC_I2C->CONSET = (1 << 5); // Requisitar o evento de START (UM10398, 15.7.1)
    while((LPC_I2C->STAT != 0x08) && (LPC_I2C->STAT != 0x10)); // Esperar pelo pelo fim do evento de START (bit SI, UM10398, 15.7.2)

    LPC_I2C->DAT = (endereco << 1) | 1; // Enviar o endereço do dispositivo escravo e espeficicação de leitura (R/W=1) (UM10398, 15.7.3)
    LPC_I2C->CONCLR = (1 << 3) | (1 << 5); // Limpar a sinalizações SI e START (UM10398, 15.7.6)
    
    while(LPC_I2C->STAT != 0x40){ // Esperar pela transmissão do endereço e recebimento de um ACK (estado 0x40) (UM10398, 15.10.1, tabela 237)
        if(LPC_I2C->STAT == 0x48) return; 
    }
    
    for (volatile int i = 0; i < 1000; i++);  // Delay de estabilização

    for(i=0; i<qtd; i++){
        if(i < qtd - 1){
            LPC_I2C->CONSET = (1 << 2); // // AA = 1 (ACK) (UM10398, 15.7.1)
        }else{
            LPC_I2C->CONCLR = (1 << 2); // AA = 0 (NACK) (UM10398, 15.7.6)
        }

        LPC_I2C->CONCLR = (1 << 3); // Limpar a sinalização SI (UM10398, 15.7.6)

        // Esperar pelo recebimento de um byte  (o estado deve assumir 0x50 ou 0x58) (UM10398, 15.10.2, tabela 237)
        while((LPC_I2C->STAT != 0x50) && (LPC_I2C->STAT != 0x58));
        


        dados[i] = LPC_I2C->DAT; // Ler o dado recebido (UM10398, 15.7.3)
    }

    LPC_I2C->CONSET = (1 << 4); // Requisitar o evento de STOP (UM10398, 15.7.1)
    LPC_I2C->CONCLR = (1 << 3); // Limpar a sinalização SI (UM10398, 15.7.6)
}

unsigned char i2cReadReg(unsigned char addr, unsigned char reg){
    unsigned char data;

    i2cSend(addr, &reg, 1); // Enviar o endereço do registrador
    i2cReceive(addr, &data, 1); // Ler o dado do registrador

    return data;
}

void i2cWriteReg(unsigned char addr, unsigned char reg, unsigned char data){
    unsigned char buffer[2];

    buffer[0] = reg; // Endereço do registrador
    buffer[1] = data; // Dado a ser escrito

    i2cSend(addr, buffer, 2); // Enviar o endereço do registrador e o dado
}