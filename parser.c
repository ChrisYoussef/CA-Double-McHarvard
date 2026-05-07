#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE 100

// ===== OPCODES =====
enum {
    ADD = 0,
    SUB,
    MUL,
    MOVI,
    BEQZ,
    ANDI,
    EOR,
    BR,
    SLC,
    SRC,
    LDR,
    STR
};

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
uint16_t encodeInstruction(int opcode, int r1, int field) {
    return (opcode << 12) | (r1 << 6) | (field & 0x3F);
}

// ============================
// Map mnemonic → opcode
// ============================
int getOpcode(char *mnemonic) {

    if (strcmp(mnemonic, "ADD") == 0) return ADD;
    if (strcmp(mnemonic, "SUB") == 0) return SUB;
    if (strcmp(mnemonic, "MUL") == 0) return MUL;
    if (strcmp(mnemonic, "MOVI") == 0) return MOVI;
    if (strcmp(mnemonic, "BEQZ") == 0) return BEQZ;
    if (strcmp(mnemonic, "ANDI") == 0) return ANDI;
    if (strcmp(mnemonic, "EOR") == 0) return EOR;
    if (strcmp(mnemonic, "BR") == 0) return BR;
    if (strcmp(mnemonic, "SLC") == 0) return SLC;
    if (strcmp(mnemonic, "SRC") == 0) return SRC;
    if (strcmp(mnemonic, "LDR") == 0) return LDR;
    if (strcmp(mnemonic, "STR") == 0) return STR;

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

    int count = sscanf(line, "%s %s %s", mnemonic, arg1, arg2);

    int opcode = getOpcode(mnemonic);

    // R-TYPE instructions
    if (opcode == ADD || opcode == SUB || opcode == MUL || opcode == EOR || opcode == BR) {

        int r1 = parseRegister(arg1);
        int r2 = parseRegister(arg2);

        return encodeInstruction(opcode, r1, r2);
    }

    // I-TYPE instructions
    int r1 = parseRegister(arg1);
    int imm = parseImmediate(arg2);

    return encodeInstruction(opcode, r1, imm);
}

// ============================
// Load full program into memory
// ============================
void loadProgram(uint16_t instructionMemory[], const char *filename) {

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

        uint16_t instruction = parseLine(line);

        instructionMemory[index++] = instruction;

        printf("Parsed instruction: 0x%04X from line: %s \n", instruction, line); // Debug: Print parsed instruction
    }


    fclose(file);

    printf("Loaded %d instructions into memory\n", index);
}
/*int main() {

    // Instruction memory
    uint16_t instructionMemory[1024] = {0};

    // Load assembly program
    loadProgram(instructionMemory, "program.txt");

    printf("\n===== Instruction Memory =====\n");

    // Print loaded instructions
    for (int i = 0; i < 13; i++) {

        printf("Instruction[%d] = ", i);

        // Print as hexadecimal
        printf("0x%04X", instructionMemory[i]);

        // Print as decimal too
        printf(" (%u)", instructionMemory[i]);

        printf("\n");
    }

    return 0;
}*/