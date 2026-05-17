#include "cpu.h"


int main(void) {
    CPU cpu;
    initCPU(&cpu);
    loadProgram(&cpu, "program.txt");
    runPipeline(&cpu);
    printRegisters(&cpu);
    printInstructionMemory(&cpu);
    printDataMemory(&cpu);
    return 0;
}