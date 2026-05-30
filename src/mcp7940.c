#include "mcp7940.h"
#include "i2c.h"
#include "serial.h"

unsigned char bcdToDec(unsigned char bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

unsigned char decToBcd(unsigned char dec){
    return ((dec / 10) << 4) | (dec % 10);
}

void mcp7940Init(void){

    unsigned char sec = i2cReadReg(MCP7940_ADDR, MCP_REG_SECONDS);

    if(!(sec & 0x80)){

        sec |= 0x80;
        i2cWriteReg(MCP7940_ADDR, MCP_REG_SECONDS, sec);

    }
}

unsigned char mcpReadReg(unsigned char reg){
    return i2cReadReg(MCP7940_ADDR, reg);
}

void mcpWriteReg(unsigned char reg, unsigned char value){
    i2cWriteReg(MCP7940_ADDR, reg, value);
}

void mcpSramWrite(unsigned char addr, const unsigned char *data, unsigned char len){
    if(addr < MCP7940_SRAM_START || addr + len - 1 > MCP7940_SRAM_END){
        return;
    }

    unsigned char buffer[len + 1];
    buffer[0] = addr;
    int i = 0;
    for(i=0; i<len; i++){
        buffer[i+1] = data[i];
    }

    i2cSend(MCP7940_ADDR, buffer, len + 1);
}

void mcpSramRead(unsigned char addr, unsigned char *data, unsigned char len){
    if(addr < MCP7940_SRAM_START || addr + len - 1 > MCP7940_SRAM_END){
        return;
    }

    i2cSend(MCP7940_ADDR, &addr, 1);
    i2cReceive(MCP7940_ADDR, data, len);
}