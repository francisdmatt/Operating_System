#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    printf("\nArray of pointers to structures\n");

    // this way with typedef you can just refer to it by name alone
    // instead of with struct everywhere
    typedef struct
    {
        int pid;
        int processType;
        unsigned short pCounter;
        int isRunning;
        int isComplete;
        int executionTime;
        unsigned short processCC;
        signed short processRegisters[256];
        int pageTick;
        int frameTick;
    } processBlock;

    processBlock *queueArray[2][10]; // I believe these are null pointers
    int i = 0;
    for (i; i < 10; i++) {
    	queueArray[0][i] = NULL;
    	if (queueArray[0][i]) printf("not null\n");
    	else printf("null\n");
    }
    // this is required to allocate the space in memory
    // for each individual processBlock struct
    queueArray[0][0] = malloc(sizeof(processBlock));
    queueArray[1][0] = malloc(sizeof(processBlock));

    // some test values
    queueArray[0][0]->pid = 20;
    queueArray[0][0]->processRegisters[20] = 40;

    // some test values
    queueArray[1][0]->pid = 30;
    queueArray[1][0]->processRegisters[20] = 60;

    // Printing values using pointers int he 2D array
    printf("\nBefore Switching Pointers\n");
    printf("Array 1:\t%d\t%d\n", queueArray[0][0]->pid, queueArray[0][0]->processRegisters[20]);
    printf("Array 2:\t%d\t%d\n", queueArray[1][0]->pid, queueArray[1][0]->processRegisters[20]);

    processBlock *temp = queueArray[0][0];
    queueArray[0][0] = queueArray[1][0];
    queueArray[1][0] = NULL;

    // Printing exact same thing; since pointers switched, values changed
    printf("\nAfter Switching Pointers\n");
    printf("Array 1:\t%d\t%d\n", queueArray[0][0]->pid, queueArray[0][0]->processRegisters[20]);
    printf("Array 2:\t%d\t%d\n", queueArray[1][0]->pid, queueArray[1][0]->processRegisters[20]);


    return 0;
}