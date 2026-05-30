#ifndef SERIAL_H
#define SERIAL_H

void serialInit(void);
void serialSendChar(char c);
void serialSendString(const char *str);
int serialReadChar(void);




#endif // SERIAL_H
