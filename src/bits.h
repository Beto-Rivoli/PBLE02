#ifndef BITS_H
#define BITS_H

#define bitSet(arg, bit) ((arg) |=  (1 << (bit)))
#define bitClr(arg, bit) ((arg) &= ~(1 << (bit)))
#define bitTgl(arg, bit) ((arg) ^=  (1 << (bit)))
#define bitTst(arg, bit) ((arg) &   (1 << (bit)))

#endif // BITS_H