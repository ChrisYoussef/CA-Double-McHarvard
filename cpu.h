#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdio.h>

#define NUM_REGS 64
#define INSTR_MEM_SIZE 1024
#define DATA_MEM_SIZE 2048

// SREG flag bit positions
#define FLAG_C 4
#define FLAG_V 3
#define FLAG_N 2
#define FLAG_S 1
#define FLAG_Z 0

// Opcodes for Package 3
typedef enum {
    OP_ADD  = 0,
    OP_SUB  = 1,
    OP_MUL  = 2,
    OP_MOVI = 3,
    OP_BEQZ = 4,
    OP_ANDI = 5,
    OP_EOR  = 6,
    OP_BR   = 7,
    OP_SLC  = 8,
    OP_SRC  = 9,
    OP_LDR  = 10,
    OP_STR  = 11
} Opcode;

typedef enum {
    FORMAT_R,
    FORMAT_I,
    FORMAT_UNKNOWN
} InstructionFormat;

typedef enum{
    IF,
    ID,
    EX
}RegisterStage;

// Decoded instruction after ID stage
typedef struct {
    uint16_t raw;      // full 16-bit instruction
    uint8_t opcode;    // bits 15-12
    uint8_t r1;        // bits 11-6
    uint8_t r2;        // bits 5-0 if R-format
    int8_t imm;        // signed immediate if I-format
    uint16_t pcValue;  // PC address of this instruction
    InstructionFormat format; // R or I format
} DecodedInstruction;

// IF/ID pipeline register
typedef struct {
    uint8_t valid;
    uint16_t rawInstruction;
    uint16_t pcValue;
} IF_ID_Register;

// ID/EX pipeline register
typedef struct {
    uint8_t valid;
    DecodedInstruction instr;
    int8_t operand1;
    int8_t operand2;
} ID_EX_Register;


// Full CPU state
// NOTE: instructionMemory and dataMemory have been moved to memory.c
//       and are accessed through the memory functions below.
typedef struct {
    int8_t GPR[NUM_REGS];   // R0 to R63
    uint8_t SREG;            // Status register
    uint16_t PC;             // Program counter

    uint32_t clockCycle;

    IF_ID_Register if_id;
    ID_EX_Register id_ex;

    uint16_t instructionCount; // number of loaded instructions
    uint16_t dataCount; // number of loaded data items

    uint8_t stall;
    uint8_t branchTaken;
    uint16_t branchTarget;
    uint8_t skipFetch;

} CPU;

// Memory arrays live in memory.c — visible to all other files via extern
extern uint16_t instructionMemory[INSTR_MEM_SIZE];
extern int8_t   dataMemory[DATA_MEM_SIZE];
extern uint8_t  dataMemoryWritten[DATA_MEM_SIZE];


// =====================
// Function Prototypes
// =====================

// CPU setup
void initCPU(CPU *cpu);

// Parser / loader
void loadProgram(CPU *cpu, const char *filename);

// Decode
DecodedInstruction decodeInstruction(uint16_t raw, uint16_t pcValue);

// Pipeline
void runPipeline(CPU *cpu);
void simulateCycle(CPU *cpu);


//hazards
int detectDataHazard(CPU *cpu);
int instructionWritesRegister(DecodedInstruction instr);
int instructionReadsR1(DecodedInstruction instr);
int instructionReadsR2(DecodedInstruction instr);

// Stages
void fetchStage(CPU *cpu);
void decodeStage(CPU *cpu);
void executeStage(CPU *cpu);

// Registers
int8_t readRegister(CPU *cpu, uint8_t regNum);
void writeRegister(CPU *cpu, uint8_t regNum, int8_t value);

// SREG / flags
void setFlag(CPU *cpu, uint8_t flag, uint8_t value);
uint8_t getFlag(CPU *cpu, uint8_t flag);
void updateNZFlags(CPU *cpu, int8_t result);
void updateSignFlag(CPU *cpu);
void updateAddFlags(CPU *cpu, int8_t a, int8_t b, int16_t result);
void updateSubFlags(CPU *cpu, int8_t a, int8_t b, int16_t result);

// Memory
uint16_t fetchInstruction(uint16_t address);
int8_t loadData(uint16_t address);
void storeData(CPU *cpu, uint16_t address, int8_t value);
uint16_t getPC(CPU *cpu);
void setPC(CPU *cpu, uint16_t value);
void incrementPC(CPU *cpu);

// Execution
void executeInstruction(CPU *cpu, ID_EX_Register *stage);

// Printing
void printCycle(CPU *cpu);
void printFetchStage(CPU *cpu, uint16_t pc, uint16_t rawInstruction);
void printFetchStalled(CPU *cpu);
void printFetchSkipped(CPU *cpu);
void printFetchEmpty(CPU *cpu);
void printDecodeStage(CPU *cpu, DecodedInstruction instr, int8_t operand1, int8_t operand2);
void printDecodeEmpty(void);
void printDecodeHazard(CPU *cpu);
void printExecuteStage(CPU *cpu, ID_EX_Register *stage);
void printExecuteEmpty(void);
void printBranchFlush(CPU *cpu);
void printRegisters(CPU *cpu);
void printInstructionMemory(CPU *cpu);
void printDataMemory(CPU *cpu);

#endif
