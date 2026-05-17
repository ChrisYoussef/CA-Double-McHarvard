#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cpu.h"

#define MAX_LINE 100

// ============================
// Convert "R5" → 5
// ============================
int parseRegister(char *reg) {
    return atoi(reg + 1);
}

// ============================
// Parse immediate
// ============================
int parseImmediate(char *immStr) {
    return atoi(immStr);
}

// ============================
// Encode instruction (core)
// ============================
static uint16_t encodeInstructionBits(int opcode, int r1, int field) {
    return (opcode << 12) | (r1 << 6) | (field & 0x3F);
}

// ============================
// Map mnemonic → opcode
// ============================
int getOpcode(char *mnemonic) {

    if (strcmp(mnemonic, "ADD") == 0) return OP_ADD;
    if (strcmp(mnemonic, "SUB") == 0) return OP_SUB;
    if (strcmp(mnemonic, "MUL") == 0) return OP_MUL;
    if (strcmp(mnemonic, "MOVI") == 0) return OP_MOVI;
    if (strcmp(mnemonic, "BEQZ") == 0) return OP_BEQZ;
    if (strcmp(mnemonic, "ANDI") == 0) return OP_ANDI;
    if (strcmp(mnemonic, "EOR") == 0) return OP_EOR;
    if (strcmp(mnemonic, "BR") == 0) return OP_BR;
    if (strcmp(mnemonic, "SLC") == 0) return OP_SLC;
    if (strcmp(mnemonic, "SRC") == 0) return OP_SRC;
    if (strcmp(mnemonic, "LDR") == 0) return OP_LDR;
    if (strcmp(mnemonic, "STR") == 0) return OP_STR;

    printf("Error: Unknown instruction %s\n", mnemonic);
    exit(1);
}

// ============================
// Parse one line → 16-bit instruction
// ============================
uint16_t parseLine(char *line) {

    char mnemonic[10];
    char arg1[10];
    char arg2[10];

    int count = sscanf(line, "%9s %9s %9s", mnemonic, arg1, arg2);
    if (count != 3) {
        printf("Error: Invalid instruction format: %s\n", line);
        exit(1);
    }

    int opcode = getOpcode(mnemonic);

    // R-TYPE instructions
    if (opcode == OP_ADD || opcode == OP_SUB || opcode == OP_MUL || opcode == OP_EOR || opcode == OP_BR) {

        int r1 = parseRegister(arg1);
        int r2 = parseRegister(arg2);

        return encodeInstructionBits(opcode, r1, r2);
    }

    // I-TYPE instructions
    int r1 = parseRegister(arg1);
    int imm = parseImmediate(arg2);

    return encodeInstructionBits(opcode, r1, imm);
}

// ============================
// Load full program into memory
// ============================
void loadProgram(CPU *cpu, const char *filename) {

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Cannot open file\n");
        exit(1);
    }

    char line[MAX_LINE];
    int index = 0;

    while (fgets(line, sizeof(line), file)) {

        // Skip empty lines
        if (strlen(line) <= 1) continue;

        if (index >= INSTR_MEM_SIZE) {
            printf("Error: Program too large for instruction memory\n");
            fclose(file);
            exit(1);
        }

        uint16_t instruction = parseLine(line);
        instructionMemory[index++] = instruction;

        printf("Parsed instruction: 0x%04X from line: %s \n", instruction, line); // Debug: Print parsed instruction
    }


    fclose(file);
    cpu->instructionCount = (uint16_t)index;

    printf("Loaded %d instructions into memory\n", index);
}
