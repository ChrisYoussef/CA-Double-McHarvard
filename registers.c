#include <stdint.h>
#include "cpu.h"

int8_t readRegister(CPU *cpu, uint8_t regNum) {
    return cpu->GPR[regNum];
}

void writeRegister(CPU *cpu, uint8_t regNum, int8_t value) {
    cpu->GPR[regNum] = value;
}