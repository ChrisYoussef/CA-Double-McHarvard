#include "cpu.h"




static InstructionFormat getFormat(uint8_t opcode) {
    switch (opcode) {
        // R-Format Opcodes
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_EOR:
        case OP_BR:
            return FORMAT_R;
            

        case OP_MOVI:
        case OP_BEQZ:
        case OP_ANDI:
        case OP_SLC:
        case OP_SRC:
        case OP_LDR:
        case OP_STR:
            return FORMAT_I;
            
        default:
            return FORMAT_UNKNOWN;
    }
}

static int8_t signExtend6Bit(uint8_t imm6) {

    imm6 = imm6 & 0x3F; 
    

    if (imm6 & 0x20) {

        return (int8_t)(imm6 | 0xC0); 
    } else {

        return (int8_t)imm6;
    }
}


DecodedInstruction decodeInstruction(uint16_t raw, uint16_t pcValue) {
    DecodedInstruction decoded;
    decoded.raw = raw;
    decoded.pcValue = pcValue;
    decoded.opcode = (raw >> 12) & 0x0F;
    decoded.r1 = (raw >> 6) & 0x3F;
    uint8_t lowest_6_bits = raw & 0x3F;

    InstructionFormat format = getFormat(decoded.opcode);
    decoded.format = format;

    if (format == FORMAT_R) {
        decoded.r2 = lowest_6_bits;
        decoded.imm = 0; 
    } 
    else if (format == FORMAT_I) {
        decoded.r2 = 0;  

        if (decoded.opcode == OP_SLC || decoded.opcode == OP_SRC) {
            decoded.imm = (int8_t)lowest_6_bits; 
        } else {
            decoded.imm = signExtend6Bit(lowest_6_bits);
        }
    } 
    else {
        // Safety fallback for unknown opcodes
        decoded.r2 = 0;
        decoded.imm = 0;
    }

    return decoded;
}