#ifndef IO_H
#define IO_H

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0

typedef enum{
    PIO0_0 =  (0 << 5) | 0,
    PIO0_1 =  (0 << 5) | 1,
    PIO0_2 =  (0 << 5) | 2,
    PIO0_3 =  (0 << 5) | 3,
    PIO0_4 =  (0 << 5) | 4,
    PIO0_5 =  (0 << 5) | 5,
    PIO0_6 =  (0 << 5) | 6,
    PIO0_7 =  (0 << 5) | 7,
    PIO0_8 =  (0 << 5) | 8,
    PIO0_9 =  (0 << 5) | 9,
    PIO0_10 = (0 << 5) | 10,
    PIO0_11 = (0 << 5) | 11,
    PIO0_12 = (0 << 5) | 12,
    PIO0_13 = (0 << 5) | 13,
    PIO0_14 = (0 << 5) | 14,
    PIO0_15 = (0 << 5) | 15,
    PIO0_16 = (0 << 5) | 16,
    PIO0_17 = (0 << 5) | 17,
    PIO0_18 = (0 << 5) | 18,
    PIO0_19 = (0 << 5) | 19,
    PIO0_20 = (0 << 5) | 20,
    PIO0_21 = (0 << 5) | 21,
    PIO0_22 = (0 << 5) | 22,
    PIO0_23 = (0 << 5) | 23,

    PIO1_0 =  (1 << 5) | 0,
    PIO1_1 =  (1 << 5) | 1,
    PIO1_2 =  (1 << 5) | 2,
    PIO1_3 =  (1 << 5) | 3,
    PIO1_4 =  (1 << 5) | 4,
    PIO1_5 =  (1 << 5) | 5,
    PIO1_6 =  (1 << 5) | 6,
    PIO1_7 =  (1 << 5) | 7,
    PIO1_8 =  (1 << 5) | 8,
    PIO1_9 =  (1 << 5) | 9,
    PIO1_10 = (1 << 5) | 10,
    PIO1_11 = (1 << 5) | 11,
    PIO1_12 = (1 << 5) | 12,
    PIO1_13 = (1 << 5) | 13,
    PIO1_14 = (1 << 5) | 14,
    PIO1_15 = (1 << 5) | 15,
    PIO1_16 = (1 << 5) | 16,
    PIO1_17 = (1 << 5) | 17,
    PIO1_18 = (1 << 5) | 18,
    PIO1_19 = (1 << 5) | 19,
    PIO1_20 = (1 << 5) | 20,
    PIO1_21 = (1 << 5) | 21,
    PIO1_22 = (1 << 5) | 22,
    PIO1_23 = (1 << 5) | 23,
    PIO1_24 = (1 << 5) | 24,
    PIO1_25 = (1 << 5) | 25,
    PIO1_26 = (1 << 5) | 26,
    PIO1_27 = (1 << 5) | 27,
    PIO1_28 = (1 << 5) | 28,
    PIO1_29 = (1 << 5) | 29,
    PIO1_31 = (1 << 5) | 31,
} pin;

//Define LEDs
#define LED0 PIO1_24
#define LED1 PIO0_6
#define LED2 PIO0_7
#define LED3 PIO1_28

//Define Buttons
#define BUTTON_S2 PIO0_22
#define BUTTON_S3 PIO0_9
#define BUTTON_S4 PIO0_8
#define BUTTON_S5 PIO1_21
#define BUTTON_S6 PIO1_31

//Define i2c pins
#define I2C_SDA PIO0_5
#define I2C_SCL PIO0_4

//Function prototypes
void pinMode(pin p, int mode);
void digitalWrite(pin p, int value);
int digitalRead(pin p);
void digitalToggle(pin p);

#endif // IO_H