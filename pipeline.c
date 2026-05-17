

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

    if (cpu->skipFetch) {
        printFetchSkipped(cpu);
        cpu->skipFetch = 0;
        return;
    }

    if (cpu->stall) {
        // freeze fetch and PC
        printFetchStalled(cpu);
        return;
    }

    if (cpu->PC >= cpu->instructionCount) {
        printFetchEmpty(cpu);
        return;
    }


    uint16_t pc = getPC(cpu);
    uint16_t rawInstruction = fetchInstruction(pc);
    cpu->if_id.valid = 1; // Assume fetch is always successful for valid PC
    cpu->if_id.rawInstruction = rawInstruction;
    cpu->if_id.pcValue = pc;
    incrementPC(cpu); // Move to next instruction for next cycle
    printFetchStage(cpu, pc, rawInstruction);

}




void decodeStage(CPU *cpu){

    if (!cpu->if_id.valid) {
        printDecodeEmpty();
        cpu->id_ex.valid = 0;
        return;
    }


    if (detectDataHazard(cpu)) {
        cpu->stall = 1;
        printDecodeHazard(cpu);

        // insert bubble into EX
        cpu->id_ex.valid = 0;

        return;
    }


    cpu->stall = 0;


    uint16_t rawInstruction=cpu ->if_id.rawInstruction;
    uint16_t pcValue=cpu ->if_id.pcValue;
    DecodedInstruction decoded = decodeInstruction(rawInstruction, pcValue);
    cpu->id_ex.valid = 1;
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
    printDecodeStage(cpu, decoded, cpu->id_ex.operand1, cpu->id_ex.operand2);

    cpu->if_id.valid = 0; // Clear IF/ID after moving to ID/EX
}


void executeStage(CPU *cpu){

    if (!cpu->id_ex.valid) {
        printExecuteEmpty();
        return;
    }

    ID_EX_Register *instruction = &cpu->id_ex;
    printExecuteStage(cpu, instruction);
    executeInstruction(cpu, instruction);

    if (cpu->branchTaken) {
        printBranchFlush(cpu);
        cpu->PC = cpu->branchTarget;
        cpu->if_id.valid = 0; // Flush IF/ID
        cpu->id_ex.valid = 0; // Flush ID/EX
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

    // Execute stages in reverse order to simulate pipeline behavior
    executeStage(cpu);
    decodeStage(cpu);
    fetchStage(cpu);



    // Increment clock cycle
    cpu->clockCycle++;
}
