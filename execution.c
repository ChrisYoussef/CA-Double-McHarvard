/*
 * execute.c – Package 3: Double Big Harvard combo large arithmetic shifts
 * CSEN601 – Spring 26
 *
 * Contains the per-opcode execute functions and the main dispatcher:
 *
 *   executeADD()   executeANDI()   executeSRC()
 *   executeSUB()   executeEOR()    executeBEQZ()
 *   executeMUL()   executeSLC()    executeBR()
 *   executeMOVI()  executeLDR()    executeSTR()
 *
 *   executeInstruction()  ← dispatches to the functions above
 *
 * Flag helpers (setFlag, getFlag, updateNZFlags, updateAddFlags,
 * updateSubFlags, updateSignFlag) live in alu.c.
 *
 * Memory helpers (loadData, storeData) live in memory.c.
 */

#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  External declarations                                              */
/* ------------------------------------------------------------------ */

/* alu.c */
void    setFlag       (CPU *, uint8_t flag, uint8_t value);
uint8_t getFlag       (CPU *, uint8_t flag);
void    updateNZFlags (CPU *, int8_t result);
void    updateAddFlags(CPU *, int8_t a, int8_t b, int16_t result);
void    updateSubFlags(CPU *, int8_t a, int8_t b, int16_t result);

/* registers.c */
int8_t readRegister (CPU *, uint8_t regNum);
void   writeRegister(CPU *, uint8_t regNum, int8_t value);

/* memory.c */
int8_t loadData (CPU *, uint16_t address);
void   storeData(CPU *, uint16_t address, int8_t value);

/* ================================================================== */
/*  0 – ADD  R1, R2  →  R1 = R1 + R2                                 */
/*  Flags updated: C, V, N, S, Z                                      */
/* ================================================================== */
void executeADD(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  op2    = stage->operand2;
    uint8_t r1     = stage->instr.r1;

    int16_t wide   = (int16_t)op1 + (int16_t)op2;
    int8_t  result = (int8_t)(wide & 0xFF);

    updateAddFlags(cpu, op1, op2, wide);
    writeRegister(cpu, r1, result);

    printf("ADD  R%u(%d) + R%u(%d) = %d  "
           "| C=%d V=%d N=%d S=%d Z=%d\n",
           r1, op1, stage->instr.r2, op2, result,
           getFlag(cpu, FLAG_C), getFlag(cpu, FLAG_V),
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_S),
           getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  1 – SUB  R1, R2  →  R1 = R1 - R2                                 */
/*  Flags updated: V, N, S, Z   (C is NOT updated by SUB)            */
/* ================================================================== */
void executeSUB(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  op2    = stage->operand2;
    uint8_t r1     = stage->instr.r1;

    int16_t wide   = (int16_t)op1 - (int16_t)op2;
    int8_t  result = (int8_t)(wide & 0xFF);

    updateSubFlags(cpu, op1, op2, wide);
    writeRegister(cpu, r1, result);

    printf("SUB  R%u(%d) - R%u(%d) = %d  "
           "| V=%d N=%d S=%d Z=%d\n",
           r1, op1, stage->instr.r2, op2, result,
           getFlag(cpu, FLAG_V), getFlag(cpu, FLAG_N),
           getFlag(cpu, FLAG_S), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  2 – MUL  R1, R2  →  R1 = (R1 * R2) low 8 bits                   */
/*  Flags updated: N, Z                                               */
/* ================================================================== */
void executeMUL(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  op2    = stage->operand2;
    uint8_t r1     = stage->instr.r1;

    int16_t wide   = (int16_t)op1 * (int16_t)op2;
    int8_t  result = (int8_t)(wide & 0xFF);

    updateNZFlags(cpu, result);
    writeRegister(cpu, r1, result);

    printf("MUL  R%u(%d) * R%u(%d) = %d  "
           "| N=%d Z=%d\n",
           r1, op1, stage->instr.r2, op2, result,
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  3 – MOVI  R1, IMM  →  R1 = IMM                                   */
/*  No flag updates.                                                  */
/* ================================================================== */
void executeMOVI(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  imm = stage->instr.imm;
    uint8_t r1  = stage->instr.r1;

    writeRegister(cpu, r1, imm);

    printf("MOVI R%u = %d\n", r1, imm);
}

/* ================================================================== */
/*  5 – ANDI  R1, IMM  →  R1 = R1 & IMM                              */
/*  Flags updated: N, Z                                               */
/* ================================================================== */
void executeANDI(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  imm    = stage->instr.imm;
    uint8_t r1     = stage->instr.r1;

    int8_t  result = (int8_t)(op1 & imm);

    updateNZFlags(cpu, result);
    writeRegister(cpu, r1, result);

    printf("ANDI R%u(%d) & %d = %d  "
           "| N=%d Z=%d\n",
           r1, op1, imm, result,
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  6 – EOR  R1, R2  →  R1 = R1 XOR R2                               */
/*  Flags updated: N, Z                                               */
/* ================================================================== */
void executeEOR(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  op2    = stage->operand2;
    uint8_t r1     = stage->instr.r1;

    int8_t  result = (int8_t)(op1 ^ op2);

    updateNZFlags(cpu, result);
    writeRegister(cpu, r1, result);

    printf("EOR  R%u(%d) ^ R%u(%d) = %d  "
           "| N=%d Z=%d\n",
           r1, op1, stage->instr.r2, op2, result,
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  8 – SLC  R1, IMM  →  R1 = (R1 << IMM) | (R1 >> (8 - IMM))       */
/*  Shift Left Circular.  IMM is always positive (spec).             */
/*  Flags updated: N, Z                                               */
/* ================================================================== */
void executeSLC(CPU *cpu, ID_EX_Register *stage)
{
    uint8_t ua     = (uint8_t)stage->operand1;
    uint8_t shamt  = (uint8_t)(stage->instr.imm & 0x07);  /* clamp 0-7 */
    uint8_t r1     = stage->instr.r1;

    uint8_t res_u  = (shamt == 0) ? ua
                   : (uint8_t)((ua << shamt) | (ua >> (8u - shamt)));
    int8_t  result = (int8_t)res_u;

    updateNZFlags(cpu, result);
    writeRegister(cpu, r1, result);

    printf("SLC  R%u(0x%02X) <<< %u = 0x%02X  "
           "| N=%d Z=%d\n",
           r1, ua, shamt, res_u,
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  9 – SRC  R1, IMM  →  R1 = (R1 >> IMM) | (R1 << (8 - IMM))       */
/*  Shift Right Circular.  IMM is always positive (spec).            */
/*  Flags updated: N, Z                                               */
/* ================================================================== */
void executeSRC(CPU *cpu, ID_EX_Register *stage)
{
    uint8_t ua     = (uint8_t)stage->operand1;
    uint8_t shamt  = (uint8_t)(stage->instr.imm & 0x07);
    uint8_t r1     = stage->instr.r1;

    uint8_t res_u  = (shamt == 0) ? ua
                   : (uint8_t)((ua >> shamt) | (ua << (8u - shamt)));
    int8_t  result = (int8_t)res_u;

    updateNZFlags(cpu, result);
    writeRegister(cpu, r1, result);

    printf("SRC  R%u(0x%02X) >>> %u = 0x%02X  "
           "| N=%d Z=%d\n",
           r1, ua, shamt, res_u,
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_Z));
}

/* ================================================================== */
/*  4 – BEQZ  R1, IMM  →  if (R1 == 0)  PC = pcValue + 1 + IMM      */
/*                                                                      */
/*  pcValue is stored from the Fetch stage (the address of THIS        */
/*  instruction).  PC was already incremented by 1 during Fetch, so   */
/*  "PC + 1" in the spec is simply  pcValue + 1.                      */
/*  No flag updates.                                                   */
/* ================================================================== */
void executeBEQZ(CPU *cpu, ID_EX_Register *stage)
{
    int8_t   op1  = stage->operand1;
    uint8_t  r1   = stage->instr.r1;
    int8_t   imm  = stage->instr.imm;

    if (op1 == 0) {
        uint16_t target = (uint16_t)(stage->instr.pcValue + 1 + (int16_t)imm);
        cpu->PC = target;
        printf("BEQZ R%u(%d) == 0  →  TAKEN, PC = %u\n", r1, op1, target);
    } else {
        printf("BEQZ R%u(%d) != 0  →  NOT TAKEN\n", r1, op1);
    }
}

/* ================================================================== */
/*  7 – BR  R1, R2  →  PC = R1 || R2                                  */
/*  R1 = most-significant byte, R2 = least-significant byte.          */
/*  No flag updates.                                                   */
/* ================================================================== */
void executeBR(CPU *cpu, ID_EX_Register *stage)
{
    int8_t  op1    = stage->operand1;
    int8_t  op2    = stage->operand2;
    uint8_t r1     = stage->instr.r1;
    uint8_t r2     = stage->instr.r2;

    uint16_t target = (uint16_t)(((uint8_t)op1 << 8) | (uint8_t)op2);
    cpu->PC = target;

    printf("BR   PC = R%u(0x%02X) || R%u(0x%02X)  →  PC = 0x%04X\n",
           r1, (uint8_t)op1, r2, (uint8_t)op2, target);
}

/* ================================================================== */
/*  10 – LDR  R1, ADDRESS  →  R1 = MEM[ADDRESS]                       */
/*  ADDRESS is the 6-bit immediate (zero-extended).                   */
/*  No flag updates.                                                   */
/* ================================================================== */
void executeLDR(CPU *cpu, ID_EX_Register *stage)
{
    uint16_t addr   = (uint16_t)(uint8_t)stage->instr.imm;
    int8_t   val    = loadData(cpu, addr);
    uint8_t  r1     = stage->instr.r1;

    writeRegister(cpu, r1, val);

    printf("LDR  R%u = MEM[%u] = %d\n", r1, addr, val);
}

/* ================================================================== */
/*  11 – STR  R1, ADDRESS  →  MEM[ADDRESS] = R1                       */
/*  ADDRESS is the 6-bit immediate (zero-extended).                   */
/*  No flag updates.                                                   */
/* ================================================================== */
void executeSTR(CPU *cpu, ID_EX_Register *stage)
{
    uint16_t addr = (uint16_t)(uint8_t)stage->instr.imm;
    int8_t   op1  = stage->operand1;
    uint8_t  r1   = stage->instr.r1;

    storeData(cpu, addr, op1);

    printf("STR  MEM[%u] = R%u(%d)\n", addr, r1, op1);
}

/* ================================================================== */
/*  executeInstruction – main dispatch                                 */
/*                                                                      */
/*  Called from executeStage() in execution.c every time a valid      */
/*  instruction reaches the EX pipeline stage.                        */
/* ================================================================== */
void executeInstruction(CPU *cpu, ID_EX_Register *stage)
{
    if (!stage->valid) return;

    printf("  [EX] ");

    switch ((Opcode)stage->instr.opcode)
    {
        case OP_ADD:  executeADD (cpu, stage); break;
        case OP_SUB:  executeSUB (cpu, stage); break;
        case OP_MUL:  executeMUL (cpu, stage); break;
        case OP_MOVI: executeMOVI(cpu, stage); break;
        case OP_BEQZ: executeBEQZ(cpu, stage); break;
        case OP_ANDI: executeANDI(cpu, stage); break;
        case OP_EOR:  executeEOR (cpu, stage); break;
        case OP_BR:   executeBR  (cpu, stage); break;
        case OP_SLC:  executeSLC (cpu, stage); break;
        case OP_SRC:  executeSRC (cpu, stage); break;
        case OP_LDR:  executeLDR (cpu, stage); break;
        case OP_STR:  executeSTR (cpu, stage); break;

        default:
            printf("UNKNOWN opcode %u – instruction skipped\n",
                   stage->instr.opcode);
            break;
    }
}