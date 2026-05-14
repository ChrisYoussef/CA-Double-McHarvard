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
 *         0  0  0  C  V  N  S  Z
 *   FLAG_C = 4 | FLAG_V = 3 | FLAG_N = 2 | FLAG_S = 1| FLAG_Z = 0
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
 