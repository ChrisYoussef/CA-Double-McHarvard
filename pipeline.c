

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

void fetchStage(CPU *cpu, IF_ID_Register *new_if_id){

    if (cpu->skipFetch) {
        printFetchSkipped(cpu);
        cpu->skipFetch = 0;
        new_if_id->valid = 0;
        return;
    }

    if (cpu->stall) {
        // freeze fetch and PC
        printFetchStalled(cpu);
        return;
    }

    if (cpu->PC >= cpu->instructionCount) {
        printFetchEmpty(cpu);
        new_if_id->valid = 0;
        return;
    }


    uint16_t pc = getPC(cpu);
    uint16_t rawInstruction = fetchInstruction(pc);
    new_if_id->valid = 1; // Assume fetch is always successful for valid PC
    new_if_id->rawInstruction = rawInstruction;
    new_if_id->pcValue = pc;
    incrementPC(cpu); // Move to next instruction for next cycle
    printFetchStage(cpu, pc, rawInstruction);

}




void decodeStage(CPU *cpu, IF_ID_Register *old_if_id,
                 ID_EX_Register *old_id_ex, ID_EX_Register *new_id_ex){

    if (!old_if_id->valid) {
        printDecodeEmpty();
        new_id_ex->valid = 0;
        return;
    }


    if (detectDataHazard(old_if_id, old_id_ex)) {
        cpu->stall = 1;
        printDecodeHazard(old_if_id, old_id_ex);

        // insert bubble into EX
        new_id_ex->valid = 0;

        return;
    }


    cpu->stall = 0;


    uint16_t rawInstruction=old_if_id->rawInstruction;
    uint16_t pcValue=old_if_id->pcValue;
    DecodedInstruction decoded = decodeInstruction(rawInstruction, pcValue);
    new_id_ex->valid = 1;
    new_id_ex->instr = decoded;
    if (new_id_ex->valid) {
        new_id_ex->operand1 = cpu->GPR[decoded.r1];
        if(decoded.format == FORMAT_R){
            new_id_ex->operand2 = cpu->GPR[decoded.r2];
        }
        else if(decoded.format == FORMAT_I){
            new_id_ex->operand2 = decoded.imm;
        }
    }
    printDecodeStage(cpu, decoded, new_id_ex->operand1, new_id_ex->operand2);
}


void executeStage(CPU *cpu, ID_EX_Register *old_id_ex){

    if (!old_id_ex->valid) {
        printExecuteEmpty();
        return;
    }

    printExecuteStage(cpu, old_id_ex);
    executeInstruction(cpu, old_id_ex);

    if (cpu->branchTaken) {
        printBranchFlush(cpu);
        cpu->PC = cpu->branchTarget;
        cpu->skipFetch = 1;   // Fetch target in the next clock cycle
        cpu->branchTaken = 0; // Reset branch flag
    }
}

void runPipeline(CPU *cpu) {
    while (cpu->clockCycle < 1000) { // Arbitrary large number to prevent infinite loops
        simulateCycle(cpu);

        // Check for pipeline completion: no valid instructions in IF/ID and ID/EX
        if (!cpu->if_id.valid && !cpu->id_ex.valid &&
            !cpu->skipFetch && cpu->PC >= cpu->instructionCount) {
            break;
        }
    }
}

void simulateCycle(CPU *cpu) {


    printCycle(cpu);

    IF_ID_Register old_if_id = cpu->if_id;
    ID_EX_Register old_id_ex = cpu->id_ex;
    IF_ID_Register new_if_id = {0};
    ID_EX_Register new_id_ex = {0};

    cpu->stall = detectDataHazard(&old_if_id, &old_id_ex);

    // Stages run in visible pipeline order. They still read old pipeline
    // registers and write next-cycle registers, so commit happens below.
    fetchStage(cpu, &new_if_id);
    decodeStage(cpu, &old_if_id, &old_id_ex, &new_id_ex);
    executeStage(cpu, &old_id_ex);

    if (cpu->stall) {
        new_if_id = old_if_id;
    }

    if (cpu->skipFetch) {
        new_if_id.valid = 0;
        new_id_ex.valid = 0;
    }

    cpu->if_id = new_if_id;
    cpu->id_ex = new_id_ex;


    // Increment clock cycle
    cpu->clockCycle++;
}
