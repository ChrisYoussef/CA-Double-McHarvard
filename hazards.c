#include "cpu.h"




int detectDataHazard(IF_ID_Register *if_id, ID_EX_Register *id_ex) {
    if (!if_id->valid || !id_ex->valid)
        return 0;

    DecodedInstruction current =
        decodeInstruction(if_id->rawInstruction, if_id->pcValue);

    DecodedInstruction previous = id_ex->instr;

    if (!instructionWritesRegister(previous))
        return 0;

    uint8_t dest = previous.r1;

    if (instructionReadsR1(current) && current.r1 == dest)
        return 1;

    if (instructionReadsR2(current) && current.r2 == dest)
        return 1;

    return 0;
}


int instructionWritesRegister(DecodedInstruction instr) {
    switch (instr.opcode) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_MOVI:
        case OP_ANDI:
        case OP_EOR:
        case OP_SLC:
        case OP_SRC:
        case OP_LDR:
            return 1;

        default:
            return 0;
    }
}

int instructionReadsR1(DecodedInstruction instr) {
    switch (instr.opcode) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_BEQZ:
        case OP_ANDI:
        case OP_EOR:
        case OP_BR:
        case OP_SLC:
        case OP_SRC:
        case OP_STR:
            return 1;

        case OP_MOVI:
            return 0;

        default:
            return 0;
    }
}

int instructionReadsR2(DecodedInstruction instr) {
    switch (instr.opcode) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_EOR:
        case OP_BR:
            return 1;

        default:
            return 0;
    }
}
