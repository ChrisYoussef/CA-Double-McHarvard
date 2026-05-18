#include "cpu.h"

static const char *opcodeName(uint8_t opcode)
{
    switch ((Opcode)opcode)
    {
    case OP_ADD:
        return "ADD";
    case OP_SUB:
        return "SUB";
    case OP_MUL:
        return "MUL";
    case OP_MOVI:
        return "MOVI";
    case OP_BEQZ:
        return "BEQZ";
    case OP_ANDI:
        return "ANDI";
    case OP_EOR:
        return "EOR";
    case OP_BR:
        return "BR";
    case OP_SLC:
        return "SLC";
    case OP_SRC:
        return "SRC";
    case OP_LDR:
        return "LDR";
    case OP_STR:
        return "STR";
    default:
        return "UNKNOWN";
    }
}

static int instructionUsesUnsignedImmediate(DecodedInstruction instr)
{
    return instr.opcode == OP_LDR || instr.opcode == OP_STR ||
           instr.opcode == OP_SLC || instr.opcode == OP_SRC;
}

static void printDecodedInstruction(DecodedInstruction instr)
{
    /* Print assembly exactly as in instruction memory followed by raw and pc */
    if (instr.format == FORMAT_R)
    {
        printf("%s R%u R%u, raw=0x%04X, pc = %u",
               opcodeName(instr.opcode), instr.r1, instr.r2,
               instr.raw, instr.pcValue);
    }
    else if (instr.format == FORMAT_I)
    {
        int immediate = instructionUsesUnsignedImmediate(instr)
                      ? (int)(instr.imm & 0x3F)
                      : (int)instr.imm;

        printf("%s R%u %d, raw=0x%04X, pc = %u",
               opcodeName(instr.opcode), instr.r1, immediate,
               instr.raw, instr.pcValue);
    }
    else
    {
        printf("%s, raw=0x%04X, pc = %u",
               opcodeName(instr.opcode), instr.raw, instr.pcValue);
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
        if (instructionUsesUnsignedImmediate(instr))
        {
            printf("  [ID] output: operand1=R%u(%d), immediate=%u\n",
                   instr.r1, operand1, (unsigned)(operand2 & 0x3F));
        }
        else
        {
            printf("  [ID] output: operand1=R%u(%d), immediate=%d\n",
                   instr.r1, operand1, operand2);
        }
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

void printDecodeHazard(IF_ID_Register *if_id, ID_EX_Register *id_ex)
{
    DecodedInstruction current =
        decodeInstruction(if_id->rawInstruction, if_id->pcValue);

    printf("  [ID] stalled: data hazard while decoding ");
    printDecodedInstruction(current);

    if (id_ex->valid)
    {
        printf(" after ");
        printDecodedInstruction(id_ex->instr);
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
        if (instructionUsesUnsignedImmediate(stage->instr))
        {
            printf(" | operand1=%d immediate=%u\n",
                   stage->operand1, (unsigned)(stage->operand2 & 0x3F));
        }
        else
        {
            printf(" | operand1=%d immediate=%d\n",
                   stage->operand1, stage->operand2);
        }
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

void printInstructionMemory(CPU *cpu)
{
    printf("\n===== Instruction Memory =====\n");
    printf("Index | Binary           | Hex    | Decimal\n");
    printf("------+------------------+--------+--------\n");

    for (int i = 0; i < cpu->instructionCount; i++)
    {
        uint16_t value = instructionMemory[i];
        char binary[17];

        for (int bit = 15; bit >= 0; bit--)
        {
            binary[15 - bit] = (value & (1u << bit)) ? '1' : '0';
        }
        binary[16] = '\0';

        printf("[%3d] | %s | 0x%04X | %5u\n",
               i, binary, value, value);
    }
}

void printDataMemory(CPU *cpu)
{
    printf("\n===== Data Memory =====\n");
    (void)cpu;

    for (int i = 0; i < DATA_MEM_SIZE; i++)
    {
        if (dataMemoryWritten[i])
        {
            printf("[%d] = %d\n", i, dataMemory[i]);
        }
    }
}
