#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

int8_t readRegister(CPU *cpu, uint8_t regNum) {
    return cpu->GPR[regNum];
}

void writeRegister(CPU *cpu, uint8_t regNum, int8_t value) {
    if (regNum >= NUM_REGS) {
        printf("    [REG] ERROR: R%u is out of range\n", regNum);
        return;
    }

    int8_t oldValue = cpu->GPR[regNum];
    cpu->GPR[regNum] = value;
    printf("    [REG] R%u: %d -> %d\n", regNum, oldValue, value);
}
