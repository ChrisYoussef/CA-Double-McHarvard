/*
 * memory.c – Package 3: Double Big Harvard combo large arithmetic shifts
 * CSEN601 – Spring 26
 *
 * Person 5 responsibility: Memory + PC Utilities
 *
 * The two memory arrays are defined here (not inside the CPU struct).
 * They are declared extern in cpu.h so all other files can access them.
 *
 * Functions implemented:
 *   initCPU()
 *   fetchInstruction()
 *   loadData()
 *   storeData()
 *   getPC()
 *   setPC()
 *   incrementPC()
 *   validateInstructionAddress()
 *   validateDataAddress()
 */

 #include "cpu.h"
 #include <stdint.h>
 #include <stdio.h>
 #include <string.h>
 
 /* ================================================================== */
 /*  Memory arrays – defined here, extern in cpu.h                     */
 /* ================================================================== */
 
 uint16_t instructionMemory[INSTR_MEM_SIZE]; // 1024 x 16-bit
 int8_t   dataMemory[DATA_MEM_SIZE];         // 2048 x 8-bit
 uint8_t  dataMemoryWritten[DATA_MEM_SIZE];  // marks addresses written by STR
 
 /* ================================================================== */
 /*  CPU Initialisation                                                 */
 /* ================================================================== */
 
 void initCPU(CPU *cpu)
 {
     memset(cpu, 0, sizeof(CPU));
     memset(instructionMemory, 0, sizeof(instructionMemory));
     memset(dataMemory,        0, sizeof(dataMemory));
     memset(dataMemoryWritten, 0, sizeof(dataMemoryWritten));
 }
 
 /* ================================================================== */
 /*  Address Validators                                                 */
 /* ================================================================== */
 
 static int validateInstructionAddress(uint16_t address)
 {
     if (address < INSTR_MEM_SIZE)
         return 1;
 
     fprintf(stderr,
             "[MEM] ERROR: instruction address %u out of range (max %u)\n",
             address, (unsigned)(INSTR_MEM_SIZE - 1));
     return 0;
 }
 
 static int validateDataAddress(uint16_t address)
 {
     if (address < DATA_MEM_SIZE)
         return 1;
 
     fprintf(stderr,
             "[MEM] ERROR: data address %u out of range (max %u)\n",
             address, (unsigned)(DATA_MEM_SIZE - 1));
     return 0;
 }
 
 /* ================================================================== */
 /*  Instruction Memory                                                 */
 /* ================================================================== */

uint16_t fetchInstruction(uint16_t address)
 {
   
     if (!validateInstructionAddress(address))
         return 0;
 
     return instructionMemory[address];
 }
 
 /* ================================================================== */
 /*  Data Memory                                                        */
 /* ================================================================== */
 
 int8_t loadData(uint16_t address)
 {
    
    if (!validateDataAddress(address))
         return 0;
 
     return dataMemory[address];
 }
 
 void storeData(CPU *cpu,  uint16_t address, int8_t value)
 {
     if (!validateDataAddress(address))
         return;
 
     dataMemory[address] = value;
     dataMemoryWritten[address] = 1;
     cpu->dataCount++;
     printf("    [MEM] dataMemory[%u] <- %d\n", address, value);
 }
 
 /* ================================================================== */
 /*  Program Counter                                                    */
 /* ================================================================== */
 
 uint16_t getPC(CPU *cpu)
 {
     return cpu->PC;
 }
 
 void setPC(CPU *cpu, uint16_t value)
 {
     cpu->PC = value;
 }
 
 void incrementPC(CPU *cpu)
 {
     cpu->PC++;
 }
 
