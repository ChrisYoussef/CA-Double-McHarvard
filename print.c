#include "cpu.h"

static const char *opcodeName(uint8_t opcode)
{
    switch ((Opcode)opcode)
    {
        case OP_ADD:  return "ADD";
        case OP_SUB:  return "SUB";
        case OP_MUL:  return "MUL";
        case OP_MOVI: return "MOVI";
        case OP_BEQZ: return "BEQZ";
        case OP_ANDI: return "ANDI";
        case OP_EOR:  return "EOR";
        case OP_BR:   return "BR";
        case OP_SLC:  return "SLC";
        case OP_SRC:  return "SRC";
        case OP_LDR:  return "LDR";
        case OP_STR:  return "STR";
        default:      return "UNKNOWN";
    }
}

static void printDecodedInstruction(DecodedInstruction instr)
{
    printf("%s raw=0x%04X pc=%u", opcodeName(instr.opcode),
           instr.raw, instr.pcValue);

    if (instr.format == FORMAT_R)
    {
        printf(" R1=R%u R2=R%u", instr.r1, instr.r2);
    }
    else if (instr.format == FORMAT_I)
    {
        printf(" R1=R%u IMM=%d", instr.r1, instr.imm);
    }
    else
    {
        printf(" unknown-format");
    }
}

void printCycle(CPU *cpu)
{
    printf("\n================ Cycle %u ================\n",
           cpu->clockCycle + 1);
    printf("PC before cycle: %u | SREG: 0x%02X "
           "(C=%u V=%u N=%u S=%u Z=%u)\n",
           cpu->PC, cpu->SREG,
           getFlag(cpu, FLAG_C), getFlag(cpu, FLAG_V),
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_S),
           getFlag(cpu, FLAG_Z));
}

void printFetchStage(CPU *cpu, uint16_t pc, uint16_t rawInstruction)
{
    DecodedInstruction instr = decodeInstruction(rawInstruction, pc);

    printf("  [IF] input: PC=%u\n", pc);
    printf("  [IF] output: fetched ");
    printDecodedInstruction(instr);
    printf(" | next PC=%u\n", cpu->PC);
}

void printFetchStalled(CPU *cpu)
{
    printf("  [IF] stalled: data hazard, PC remains %u\n", cpu->PC);
}

void printFetchSkipped(CPU *cpu)
{
    printf("  [IF] skipped: branch/jump flush, target PC=%u will fetch next cycle\n",
           cpu->PC);
}

void printFetchEmpty(CPU *cpu)
{
    printf("  [IF] empty: PC=%u is past loaded instruction count %u\n",
           cpu->PC, cpu->instructionCount);
}

void printDecodeStage(CPU *cpu, DecodedInstruction instr,
                      int8_t operand1, int8_t operand2)
{
    (void)cpu;

    printf("  [ID] input: ");
    printDecodedInstruction(instr);
    printf("\n");

    if (instr.format == FORMAT_R)
    {
        printf("  [ID] output: operand1=R%u(%d), operand2=R%u(%d)\n",
               instr.r1, operand1, instr.r2, operand2);
    }
    else if (instr.format == FORMAT_I)
    {
        printf("  [ID] output: operand1=R%u(%d), immediate=%d\n",
               instr.r1, operand1, operand2);
    }
    else
    {
        printf("  [ID] output: unknown instruction decoded as bubble\n");
    }
}

void printDecodeEmpty(void)
{
    printf("  [ID] empty: no valid instruction in IF/ID\n");
}

void printDecodeHazard(CPU *cpu)
{
    DecodedInstruction current =
        decodeInstruction(cpu->if_id.rawInstruction, cpu->if_id.pcValue);

    printf("  [ID] stalled: data hazard while decoding ");
    printDecodedInstruction(current);

    if (cpu->id_ex.valid)
    {
        printf(" after ");
        printDecodedInstruction(cpu->id_ex.instr);
    }

    printf(" | inserted bubble into EX\n");
}

void printExecuteStage(CPU *cpu, ID_EX_Register *stage)
{
    (void)cpu;

    printf("  [EX] input: ");
    printDecodedInstruction(stage->instr);

    if (stage->instr.format == FORMAT_R)
    {
        printf(" | operand1=%d operand2=%d\n",
               stage->operand1, stage->operand2);
    }
    else if (stage->instr.format == FORMAT_I)
    {
        printf(" | operand1=%d immediate=%d\n",
               stage->operand1, stage->operand2);
    }
    else
    {
        printf("\n");
    }
}

void printExecuteEmpty(void)
{
    printf("  [EX] empty: no valid instruction in ID/EX\n");
}

void printBranchFlush(CPU *cpu)
{
    printf("  [CTRL] branch/jump taken: PC <- %u, flushed IF/ID and ID/EX\n",
           cpu->branchTarget);
}

void printRegisters(CPU *cpu)
{
    printf("\n===== Registers =====\n");

    for (int i = 0; i < NUM_REGS; i++)
    {
        printf("R%-2d = %4d", i, cpu->GPR[i]);

        if ((i + 1) % 4 == 0)
            printf("\n");
        else
            printf("    ");
    }

    printf("PC   = %u\n", cpu->PC);
    printf("SREG = 0x%02X (C=%u V=%u N=%u S=%u Z=%u)\n",
           cpu->SREG,
           getFlag(cpu, FLAG_C), getFlag(cpu, FLAG_V),
           getFlag(cpu, FLAG_N), getFlag(cpu, FLAG_S),
           getFlag(cpu, FLAG_Z));
}

void printInstructionMemory()
{
    printf("\n===== Instruction Memory =====\n");

    for (int i = 0; i < INSTR_MEM_SIZE; i++)
    {
        if (instructionMemory[i] != 0)
        {
            printf("[%d] = %u\n", i, instructionMemory[i]);
        }
    }
}

void printDataMemory()
{
    printf("\n===== Data Memory =====\n");

    for (int i = 0; i < DATA_MEM_SIZE; i++)
    {
        printf("[%d] = %d\n", i, dataMemory[i]);
    }
}
