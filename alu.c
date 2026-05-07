/*
 * alu.c – Package 3: Double Big Harvard combo large arithmetic shifts
 * CSEN601 – Spring 26
 *
 * Implements all ALU-related functions declared in cpu.h:
 *   setFlag / getFlag
 *   updateNZFlags / updateSignFlag
 *   updateAddFlags / updateSubFlags
 *   executeInstruction   ← main ALU dispatch (called from EX stage)
 *
 * SREG bit layout as defined in cpu.h:
 *   bit:  7  6  5  4  3  2  1  0
 *         0  0  0  Z  S  N  V  C
 *   FLAG_C = 0 | FLAG_V = 1 | FLAG_N = 2 | FLAG_S = 3 | FLAG_Z = 4
 *
 * Flag update rules (spec):
 *   C  – ADD only
 *   V  – ADD, SUB
 *   N  – ADD, SUB, MUL, ANDI, EOR, SLC, SRC
 *   S  – ADD, SUB        (always S = N xor V)
 *   Z  – ADD, SUB, MUL, ANDI, EOR, SLC, SRC
 */

#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

/* ================================================================== */
/*  executeInstruction – main ALU dispatch                             */
/*                                                                      */
/*  Called from executeStage() in the EX pipeline step.               */
/*  The ID/EX register already contains:                              */
/*    stage->instr.r1      – destination / first-source register idx  */
/*    stage->instr.r2      – second source register idx (R-format)    */
/*    stage->instr.imm     – signed immediate (I-format)              */
/*    stage->instr.pcValue – address of THIS instruction in imem      */
/*    stage->operand1      – value already read from R[r1] in decode  */
/*    stage->operand2      – value already read from R[r2] in decode  */
/* ================================================================== */

void executeInstruction(CPU *cpu, ID_EX_Register *stage)
{
    if (!stage->valid) return;

    DecodedInstruction *instr = &stage->instr;
    int8_t  op1 = stage->operand1;   /* R[r1] */
    int8_t  op2 = stage->operand2;   /* R[r2] */
    int8_t  imm = instr->imm;
    uint8_t r1  = instr->r1;
    uint8_t r2  = instr->r2;

    printf("  [EX] ");

    switch ((Opcode)instr->opcode)
    {
    /* ----------------------------------------------------------------
     * 0: ADD  R1, R2  →  R1 = R1 + R2
     *    Flags updated: C, V, N, S, Z
     * ---------------------------------------------------------------- */
    case OP_ADD:
    {
        int16_t wide   = (int16_t)op1 + (int16_t)op2;
        int8_t  result = (int8_t)(wide & 0xFF);

        updateAddFlags(cpu, op1, op2, wide);
        writeRegister(cpu, r1, result);

        printf("ADD  R%d(%d) + R%d(%d) = %d  "
               "| C=%d V=%d N=%d S=%d Z=%d\n",
               r1, op1, r2, op2, result,
               getFlag(cpu,FLAG_C), getFlag(cpu,FLAG_V),
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_S),
               getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 1: SUB  R1, R2  →  R1 = R1 - R2
     *    Flags updated: V, N, S, Z  (C is NOT updated by SUB)
     * ---------------------------------------------------------------- */
    case OP_SUB:
    {
        int16_t wide   = (int16_t)op1 - (int16_t)op2;
        int8_t  result = (int8_t)(wide & 0xFF);

        updateSubFlags(cpu, op1, op2, wide);
        writeRegister(cpu, r1, result);

        printf("SUB  R%d(%d) - R%d(%d) = %d  "
               "| V=%d N=%d S=%d Z=%d\n",
               r1, op1, r2, op2, result,
               getFlag(cpu,FLAG_V), getFlag(cpu,FLAG_N),
               getFlag(cpu,FLAG_S), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 2: MUL  R1, R2  →  R1 = (R1 * R2) low 8 bits
     *    Flags updated: N, Z
     * ---------------------------------------------------------------- */
    case OP_MUL:
    {
        int16_t wide   = (int16_t)op1 * (int16_t)op2;
        int8_t  result = (int8_t)(wide & 0xFF);

        updateNZFlags(cpu, result);
        writeRegister(cpu, r1, result);

        printf("MUL  R%d(%d) * R%d(%d) = %d  "
               "| N=%d Z=%d\n",
               r1, op1, r2, op2, result,
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 3: MOVI  R1, IMM  →  R1 = IMM
     *    No flag updates.
     * ---------------------------------------------------------------- */
    case OP_MOVI:
    {
        writeRegister(cpu, r1, imm);

        printf("MOVI R%d = %d\n", r1, imm);
        break;
    }

    /* ----------------------------------------------------------------
     * 4: BEQZ  R1, IMM  →  if (R1 == 0)  PC = pcValue + 1 + IMM
     *    pcValue is the address stored when the instruction was fetched.
     *    The fetch stage already incremented PC by 1, so pcValue + 1
     *    is the "next instruction" address the spec refers to.
     *    No flag updates.
     * ---------------------------------------------------------------- */
    case OP_BEQZ:
    {
        if (op1 == 0)
        {
            uint16_t target = (uint16_t)(instr->pcValue + 1 + (int16_t)imm);
            cpu->PC = target;

            printf("BEQZ R%d(%d) == 0  →  TAKEN, PC = %u\n",
                   r1, op1, target);
        }
        else
        {
            printf("BEQZ R%d(%d) != 0  →  NOT TAKEN\n", r1, op1);
        }
        break;
    }

    /* ----------------------------------------------------------------
     * 5: ANDI  R1, IMM  →  R1 = R1 & IMM
     *    Flags updated: N, Z
     * ---------------------------------------------------------------- */
    case OP_ANDI:
    {
        int8_t result = (int8_t)(op1 & imm);

        updateNZFlags(cpu, result);
        writeRegister(cpu, r1, result);

        printf("ANDI R%d(%d) & %d = %d  "
               "| N=%d Z=%d\n",
               r1, op1, imm, result,
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 6: EOR  R1, R2  →  R1 = R1 xor R2
     *    Flags updated: N, Z
     * ---------------------------------------------------------------- */
    case OP_EOR:
    {
        int8_t result = (int8_t)(op1 ^ op2);

        updateNZFlags(cpu, result);
        writeRegister(cpu, r1, result);

        printf("EOR  R%d(%d) ^ R%d(%d) = %d  "
               "| N=%d Z=%d\n",
               r1, op1, r2, op2, result,
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 7: BR  R1, R2  →  PC = R1 || R2
     *    R1 = most-significant byte, R2 = least-significant byte.
     *    No flag updates.
     * ---------------------------------------------------------------- */
    case OP_BR:
    {
        uint16_t target = (uint16_t)(((uint8_t)op1 << 8) | (uint8_t)op2);
        cpu->PC = target;

        printf("BR   PC = R%d(0x%02X) || R%d(0x%02X)  →  PC = 0x%04X\n",
               r1, (uint8_t)op1, r2, (uint8_t)op2, target);
        break;
    }

    /* ----------------------------------------------------------------
     * 8: SLC  R1, IMM  →  R1 = (R1 << IMM) | (R1 >> (8 - IMM))
     *    Shift Left Circular.  IMM is always positive (spec).
     *    Flags updated: N, Z
     * ---------------------------------------------------------------- */
    case OP_SLC:
    {
        uint8_t ua    = (uint8_t)op1;
        uint8_t shamt = (uint8_t)(imm & 0x07);   /* clamp to 0-7 */
        uint8_t res_u = (shamt == 0) ? ua
                      : (uint8_t)((ua << shamt) | (ua >> (8u - shamt)));
        int8_t  result = (int8_t)res_u;

        updateNZFlags(cpu, result);
        writeRegister(cpu, r1, result);

        printf("SLC  R%d(0x%02X) <<< %u = 0x%02X  "
               "| N=%d Z=%d\n",
               r1, ua, shamt, res_u,
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 9: SRC  R1, IMM  →  R1 = (R1 >> IMM) | (R1 << (8 - IMM))
     *    Shift Right Circular.  IMM is always positive (spec).
     *    Flags updated: N, Z
     * ---------------------------------------------------------------- */
    case OP_SRC:
    {
        uint8_t ua    = (uint8_t)op1;
        uint8_t shamt = (uint8_t)(imm & 0x07);
        uint8_t res_u = (shamt == 0) ? ua
                      : (uint8_t)((ua >> shamt) | (ua << (8u - shamt)));
        int8_t  result = (int8_t)res_u;

        updateNZFlags(cpu, result);
        writeRegister(cpu, r1, result);

        printf("SRC  R%d(0x%02X) >>> %u = 0x%02X  "
               "| N=%d Z=%d\n",
               r1, ua, shamt, res_u,
               getFlag(cpu,FLAG_N), getFlag(cpu,FLAG_Z));
        break;
    }

    /* ----------------------------------------------------------------
     * 10: LDR  R1, ADDRESS  →  R1 = MEM[ADDRESS]
     *     ADDRESS is the 6-bit immediate (unsigned data memory address).
     *     No flag updates.
     * ---------------------------------------------------------------- */
    case OP_LDR:
    {
        uint16_t addr = (uint16_t)(uint8_t)imm;   /* zero-extend */
        int8_t   val  = loadData(cpu, addr);

        writeRegister(cpu, r1, val);

        printf("LDR  R%d = MEM[%u] = %d\n", r1, addr, val);
        break;
    }

    /* ----------------------------------------------------------------
     * 11: STR  R1, ADDRESS  →  MEM[ADDRESS] = R1
     *     ADDRESS is the 6-bit immediate (unsigned data memory address).
     *     No flag updates.
     * ---------------------------------------------------------------- */
    case OP_STR:
    {
        uint16_t addr = (uint16_t)(uint8_t)imm;
        storeData(cpu, addr, op1);

        printf("STR  MEM[%u] = R%d(%d)\n", addr, r1, op1);
        break;
    }

    default:
        printf("UNKNOWN opcode %u – instruction skipped\n", instr->opcode);
        break;
    }
}