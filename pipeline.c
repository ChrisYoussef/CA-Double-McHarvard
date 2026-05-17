

/************************************************************
 * pipeline.c – Package 3: Double Big Harvard combo large arithmetic shifts
 * CSEN601 – Spring 26
 *
 * Implements the main pipeline control flow and stage functions.
 * Each stage function corresponds to one of the three pipeline stages:
 *   fetchStage()   – IF stage
 *   decodeStage()  – ID stage
 *   executeStage() – EX stage
 *
 * The main pipeline loop is implemented in runPipeline(), which calls
 * simulateCycle() repeatedly until all instructions have been processed.
 ************************************************************/

#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>



/* ================================================================== */

void fetchStage(CPU *cpu){

}




void decodeStage(CPU *cpu){
    uint16_t rawInstruction=cpu ->if_id.rawInstruction;
    uint16_t pcValue=cpu ->if_id.pcValue;
    DecodedInstruction decoded = decodeInstruction(rawInstruction, pcValue);
    cpu->id_ex.valid = cpu->if_id.valid;
    cpu->id_ex.instr = decoded;
    if (cpu->id_ex.valid) {
        cpu->id_ex.operand1 = cpu->GPR[decoded.r1];
        if(decoded.format == FORMAT_R){
            cpu->id_ex.operand2 = cpu->GPR[decoded.r2];
        }
        else if(decoded.format == FORMAT_I){
            cpu->id_ex.operand2 = decoded.imm;
        }
    }
}


void executeStage(CPU *cpu){
    executeInstruction(cpu, &cpu->id_ex);
}

void runPipeline(CPU *cpu) {
    while (cpu->clockCycle < 1000) { // Arbitrary large number to prevent infinite loops
        simulateCycle(cpu);

        // Check for pipeline completion: no valid instructions in IF/ID and ID/EX
        if (!cpu->if_id.valid && !cpu->id_ex.valid) {
            break;
        }
    }
}

void simulateCycle(CPU *cpu) {

    fetchStage(cpu);
    decodeStage(cpu);
    executeStage(cpu);

    // Increment clock cycle
    cpu->clockCycle++;
}