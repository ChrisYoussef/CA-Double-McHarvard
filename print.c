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