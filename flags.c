
#include "cpu.h"
#include <stdint.h>

/* ================================================================== */
/*  SREG helpers                                                        */
/* ================================================================== */

/*
 * setFlag – write a single bit in cpu->SREG.
 *   flag  : one of FLAG_C / FLAG_V / FLAG_N / FLAG_S / FLAG_Z
 *   value : 0 or 1
 */
void setFlag(CPU *cpu, uint8_t flag, uint8_t value)
{
    if (value) // If value is 1, set the bit; if value is 0, clear the bit
        cpu->SREG |=  (uint8_t)(1u << flag);
    else
        cpu->SREG &= (uint8_t)~(1u << flag);

    /* Spec: bits 7-5 must always stay 0 */
    cpu->SREG &= 0x1F;
}

/*
 * getFlag – read a single bit from cpu->SREG.
 * Returns 0 or 1.
 */
uint8_t getFlag(CPU *cpu, uint8_t flag)
{
    return (cpu->SREG >> flag) & 1u;
}

/* ------------------------------------------------------------------ */
/*  N and Z (shared by many instructions)                              */
/* ------------------------------------------------------------------ */

/*
 * updateNZFlags – set N and Z from an 8-bit result.
 *   N = 1  if result < 0 (sign bit set)
 *   Z = 1  if result == 0
 */
void updateNZFlags(CPU *cpu, int8_t result)
{
    setFlag(cpu, FLAG_N, (result < 0) ? 1 : 0);
    setFlag(cpu, FLAG_Z, (result == 0) ? 1 : 0);
}

/* ------------------------------------------------------------------ */
/*  S flag (always derived, never set in isolation)                    */
/* ------------------------------------------------------------------ */

/*
 * updateSignFlag – recompute S = N xor V from the current SREG.
 * Always call this AFTER N and V have been updated.
 */
void updateSignFlag(CPU *cpu)
{
    uint8_t n = getFlag(cpu, FLAG_N);
    uint8_t v = getFlag(cpu, FLAG_V);
    setFlag(cpu, FLAG_S, n ^ v);
}

/* ------------------------------------------------------------------ */
/*  ADD flag bundle  (C, V, N, S, Z)                                  */
/* ------------------------------------------------------------------ */

/*
 * updateAddFlags – update all five SREG flags for an ADD.
 *
 *   a      : first  operand (original int8_t register value)
 *   b      : second operand (original int8_t register value)
 *   result : 16-bit intermediate so bit 8 is available for carry
 *
 * Carry    : bit 8 of (zero-extended a + zero-extended b)
 * Overflow : same-sign inputs produced an opposite-sign result
 */
void updateAddFlags(CPU *cpu, int8_t a, int8_t b, int16_t result)
{
    int8_t result8 = (int8_t)(result & 0xFF);

    /* Carry: check bit 8 of the unsigned sum */
    uint16_t usum = (uint16_t)(uint8_t)a + (uint16_t)(uint8_t)b;
    setFlag(cpu, FLAG_C, (usum >> 8) & 1u);

    /* Overflow: same signs in, opposite sign out */
    int sa = (a       >> 7) & 1;
    int sb = (b       >> 7) & 1;
    int sr = (result8 >> 7) & 1;
    setFlag(cpu, FLAG_V, (sa == sb && sr != sa) ? 1 : 0);

    /* N and Z from the 8-bit result */
    updateNZFlags(cpu, result8);

    /* S = N xor V */
    updateSignFlag(cpu);
}

/* ------------------------------------------------------------------ */
/*  SUB flag bundle  (V, N, S, Z)                                     */
/*  Note: the C flag is NOT updated by SUB in Package 3.              */
/* ------------------------------------------------------------------ */

/*
 * updateSubFlags – update V, N, S, Z for a SUB (a – b).
 *
 * Overflow : different signs AND result has the same sign as b (subtrahend)
 */
void updateSubFlags(CPU *cpu, int8_t a, int8_t b, int16_t result)
{
    int8_t result8 = (int8_t)(result & 0xFF);

    /* Overflow */
    int sa = (a       >> 7) & 1;
    int sb = (b       >> 7) & 1;
    int sr = (result8 >> 7) & 1;
    setFlag(cpu, FLAG_V, (sa != sb && sr == sb) ? 1 : 0);

    /* N and Z */
    updateNZFlags(cpu, result8);

    /* S = N xor V */
    updateSignFlag(cpu);
}

// Other flags (MUL, ANDI, EOR, SLC, and SRC) will use updateNZFlags() directly from the instruction implementation since they only need N and Z.

//////////////////////////////////

void printSREG(CPU *cpu) {
    printf("SREG = 0x%02X (C=%u V=%u N=%u S=%u Z=%u)\n",
           cpu->SREG,
           getFlag(cpu, FLAG_C),
           getFlag(cpu, FLAG_V),
           getFlag(cpu, FLAG_N),
           getFlag(cpu, FLAG_S),
           getFlag(cpu, FLAG_Z));
}