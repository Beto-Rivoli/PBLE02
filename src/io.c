#include "bits.h"
#include "io.h"
#include "LPC11Uxx.h"

void pinMode(pin p, int mode){
    uint8_t port = p >> 5;
    uint8_t pin  = p & 0x1F;

    if(mode == OUTPUT){
        bitSet(LPC_GPIO->DIR[port], pin);
    } else if(mode == INPUT){
        bitClr(LPC_GPIO->DIR[port], pin);
    }
}

void digitalWrite(pin p, int value){
    uint8_t port = p >> 5;
    uint8_t pin  = p & 0x1F;

    if(value == HIGH){
        bitSet(LPC_GPIO->PIN[port], pin);
    } else if(value == LOW){
        bitClr(LPC_GPIO->PIN[port], pin);
    }
}

int digitalRead(pin p){
    uint8_t port = p >> 5;
    uint8_t pin  = p & 0x1F;

    return bitTst(LPC_GPIO->PIN[port], pin) ? HIGH : LOW;
}

void digitalToggle(pin p){
    uint8_t port = p >> 5;
    uint8_t pin  = p & 0x1F;

    bitTgl(LPC_GPIO->PIN[port], pin);
}