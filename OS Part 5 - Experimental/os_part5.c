/*     Authors: Austin Blythe, Matthew Francis
 *       Acct#: cs441104
 *       Class: CSC 441, Dr. Stader
 *  Start Date: 20 Mar 2013
 * Finish Date: 8 Apr 2013
 * Description: This program builds on OS_Part 3, implementing queues
 */

#include <stdlib.h>
#include <stdio.h>

// Global Constants
const int TICKS_PER_USER = 4; // # of Ticks allowed per cycle for user

#define MAX_MEMORY 256        // Size of memory array
#define DISK_SIZE 256         // Size of the disk
#define MEMORY_LENGTH 4
#define MAX_REGISTER 4        // Number of registers in the machine
#define numberOfUsers 3       // Number of users in the system
#define numberOfProcesses 10   // Number of simultaneous processes (possible) in the system
#define true 1                // Setting keyword true to 1
#define false 0               // Setting keyword false to 0

// Global Varibles
typedef int bool;             // Create bool type as C does not have one

unsigned short CC;            // Condition code
unsigned short PC;            // Program Counter
unsigned short IR;            // Instruction Register

unsigned short opcode;        // Opcode field
unsigned short mode;          // Mode field
unsigned short reg;           // Register field
signed short address;         // Address field



// Internal
bool haltFlag;                // Controls system halt
int programClock;             // Internal programClock
int currentTick;              // Current tick that the user is on (resets to 0 when limit reached)
int currentUser;              // Current user in the RR
int currentPriority;          // Which process array is being used
int currentPosition;          // Position in the queueArray


// Structure format creation for users and O/S
typedef struct
{
    int memoryLocation;        // Starting location for that users program
    int progLength;            // Number of instructions in the user's program
    int pageTable[MAX_MEMORY / MEMORY_LENGTH];  // Page table for the user owned process(s),  allows for largest possible load.
} user;

user userArray[numberOfUsers];  // Array for creation of users + 1 to allow extra spot

typedef struct
{
    int pid;                   // Stores who owns the process
    int processType;           // Determines if it is a run(1), dmp(2), or stp(3) command
    unsigned short pCounter;   // Stores what memory location the program is currently at
    bool isRunning;            // Used to determine locked/queue status
    bool isComplete;           // Used for cleanUp method to shift queue
    int executionTime;
    unsigned short processCC;
    signed short processRegisters[MAX_REGISTER];
    int pageTick;
    int frameTick;
} processBlock;

processBlock *queueArray[2][numberOfProcesses];
int queueOneRear;
int queueTwoRear;

unsigned short disk[DISK_SIZE];              // 1D-array of short int
unsigned short mainMemory[MAX_MEMORY];       // Main memory
int usedFrames[MAX_MEMORY / MEMORY_LENGTH];  // Frame usage bit vector
signed short Registers[MAX_REGISTER];        // Registers array (0 is Accumulator)

int diskPrint[DISK_SIZE];     // locations on disk to print in dmp
int memoryPrint[MAX_MEMORY];  // locations in memory to print in dmp

char userIn[5] = {0};          // Array for user input
char *controlCommand = userIn; // Pointer to userIn array
int commandCode;               // Codes for UI commands: run (1), dmp(2), stp(3), nop(4)

// Method Declarations
int main(void);

void initializeOS();
void userInterface();

void scheduler();
void dispatcher();

void run();
void dmp();
void stp();
void nop();

void loader();
void interpreter();
void cleanUp();

void mmu(int, int);

void Fetch();
void Decode();
void Execute();

void dumpPageTable();
unsigned short convertNumber(char *);
void printBin(unsigned short);
void printHex(unsigned short);
void changeCondition(int);
bool isValidCommand();
void placeInQueue();
bool queueHasProcess();
void dumpQueues();
void promoteProcess();
void removeProcess();
void rotateProcess();

void load();
void store();
void add();
void sub();
void adr();
void sur();
void and ();
void or ();
void not();
void jmp();
void jeq();
void jgt();
void jlt();
void compare();
void clear();
void halt();

// User-defined header file:
#include "instructions.h"

// ******************** MAIN ********************

int main (void)
{
    // OS Initialization
    initializeOS();

    // User Interface loop
    while (true)
    {
        userInterface();
    }

    return 0;
}


// ******************** OPERATING SYSTEM ********************

void initializeOS()
{
    // Initialize Values
    programClock = 0;
    CC = 0;
    PC = 0;
    IR = 0;
    haltFlag = false;
    currentUser = 1;  // Starts with user 1
    currentTick = 0;  // User 1 starts with 0 ticks on their cycle
    commandCode = 0;
    queueOneRear = 0;
    queueTwoRear = 0;

    // Zero out the mainMemory and map frame locations
    // Initialize the usedFrames array so they are all available

    int i = 0;
    int j = 0;
    for (i; i < MAX_MEMORY; i++)
    {
        mainMemory[i] = 0;
        memoryPrint[i] = 0;
        if (i % MEMORY_LENGTH == 0)
        {
            usedFrames[i / MEMORY_LENGTH] = 0;
        }
    }

    // Initialize page table
    for (j = 1; j < numberOfUsers; j++)
    {
        for (i = 0; i < MAX_MEMORY / MEMORY_LENGTH; i++)
        {
            userArray[j].pageTable[i] = -1;
        }
    }

    // Zero out the disk
    for (i = 0; i < DISK_SIZE; i++)
    {
        disk[i] = 0;
        diskPrint[i] = 0;
    }

    // Set pid for processes to -1
    for (i = 0; i < numberOfProcesses; i++)
    {
        queueArray[0][i] = NULL;
        queueArray[1][i] = NULL;
    }


    // User 1 data set
    disk[0]   = 0x080A; // Location 000 // Load Immediate R0 #10
    disk[1]   = 0x1006; // Location 001 // Store R0 6
    disk[2]   = 0x0905; // Location 002 // Load Immediate R1 #5
    disk[3]   = 0x4100; // Location 003 // AddR R1
    disk[4]   = 0x1007; // Location 004 // Store R0 7
    disk[5]   = 0xF000; // Location 005 // Halt
    disk[6]   = 0x0000; // Data
    disk[7]   = 0x0000; // Data

    // User 2 data set
    disk[100] = 0x0819; // Location 100 // LOAD I R0 #25
    disk[101] = 0x1006; // Location 101 // STO R0 6
    disk[102] = 0x0905; // Location 102 // LOD I R1 #5
    disk[103] = 0x5100; // Location 103 // SUR R1
    disk[104] = 0x1007; // Location 104 // STO R0 7
    disk[105] = 0xF000; // Location 105 // HALT
    disk[106] = 0x0000; // Data
    disk[107] = 0x0000; // Data

    // User1
    userArray[1].memoryLocation = 0;
    userArray[1].progLength = 6;

    // User2
    userArray[2].memoryLocation = 100;
    userArray[2].progLength = 6;

}



// Interactive command-line user interface
void userInterface()
{
    printf("\n\n");
    // User prompt
    if (currentUser == 0) printf("Operating System: ");
    else printf("User %i: ", currentUser);

    // Get command
    fgets(controlCommand, 5, stdin);
    printf("\n");

    if (isValidCommand())
    {
        switch (commandCode)
        {
        case 1: placeInQueue(); break;
        case 2: dmp(); break;
        case 3: placeInQueue(); break;
        case 4: nop(); break;
        }

        // Schedule next execution request
        scheduler();

        // Routine cleanup after every execution; queue manipulation, process removal
        if (queueArray[currentPriority][0] != NULL)
        {
            cleanUp();
        }

        dumpQueues();

        // Cycle to the next user
        if (currentUser == numberOfUsers - 1) currentUser = 0;
        else currentUser++;
    }
    else printf("Invalid command entered\n");
}

// Process a request based on priority
void scheduler()
{
    currentTick = 0;

    if (queueHasProcess(0))
    {
        currentPriority = 0;
        dispatcher();
    }
    else if (queueHasProcess(1))
    {
        currentPriority = 1;
        dispatcher();
    }
    else
        printf("Both queues are empty; no operations performed\n");
}

// Directs users/OS based on command entered
void dispatcher()
{
    switch (queueArray[currentPriority][0]->processType)
    {
    case 1: run();
        break;
    case 3: stp();
        break;
    }
}

// Run the current user program request
void run()
{
    // Continue running a process that already started running
    if (queueArray[currentPriority][0]->isRunning == true)
    {
        PC = queueArray[currentPriority][0]->pCounter;
        CC = queueArray[currentPriority][0]->processCC;
        int i = 0;
        for (i; i < MAX_REGISTER; i++)
            Registers[i] = queueArray[currentPriority][0]->processRegisters[i];
        interpreter();
    }
    else
    {
        // First run
        loader();
        programClock++;
        currentTick++;
        PC = queueArray[currentPriority][0]->pCounter;
        queueArray[currentPriority][0]->isRunning = true;
        interpreter();
    }
}

void loader()
{
    int p, i;
    int currentFrame;
    int currentPage = 0;
    int memoryLoc;

    // Place user program pages into main memory frames
    for (p = userArray[queueArray[currentPriority][0]->pid].memoryLocation;
            p < (userArray[queueArray[currentPriority][0]->pid].progLength +
                 userArray[queueArray[currentPriority][0]->pid].memoryLocation);)
    {
        currentFrame = rand() % 64;
        memoryLoc = currentFrame * MEMORY_LENGTH;
        if (currentPage == 0)
        {
            queueArray[currentPriority][0]->pCounter = memoryLoc;

        }
        if (usedFrames[currentFrame] == 0)
        {
            for (i = 0; i < MEMORY_LENGTH; i++, p++)
            {
                mainMemory[memoryLoc + i] = disk[p];
                memoryPrint[memoryLoc + i] = queueArray[currentPriority][0]->pid;
                diskPrint[p] = 1;
            }
            usedFrames[currentFrame] = 1;
            mmu(currentPage, currentFrame);
            currentPage++;
        }
    }

}

// responsible for the machine languare interpretation and execution
void interpreter()
{   
    printf("\n");
    if (queueArray[currentPriority][0]->pid > 0) printf("User %d Running...\n", queueArray[currentPriority][0]->pid);

    while (haltFlag == false && currentTick < TICKS_PER_USER)
    {
        Fetch();
        Decode();
        mmu(-1, -1);
        Execute();
        if (queueArray[currentPriority][0]->frameTick % MEMORY_LENGTH == 0)
        {
            queueArray[currentPriority][0]->pageTick++;
            queueArray[currentPriority][0]->pCounter = userArray[queueArray[currentPriority][0]->pid].pageTable[queueArray[currentPriority][0]->pageTick] * MEMORY_LENGTH;
            PC = queueArray[currentPriority][0]->pCounter;
        }
    }

    if (haltFlag == true)
    {
        queueArray[currentPriority][0]->isComplete = true;
        dumpPageTable();
        mmu(-2, -2);
        queueArray[currentPriority][0]->isRunning = false;
    }
    else
    {
        queueArray[currentPriority][0]->pCounter = PC;
        queueArray[currentPriority][0]->processCC = CC;
        CC = 0x0000;
        int i = 0;
        for (i; i < MAX_REGISTER; i++)
            queueArray[currentPriority][0]->processRegisters[i] = Registers[i];
        Registers[i] = 0x0000;
    }
    haltFlag = false; // reset halt flag for subsequent program runs
}

// This will create a dump of the data in the program
void dmp()
{
    printf("---------------------------------\n\tDUMP START\n---------------------------------\n\n");
    programClock++;
    currentTick++;

    char reg_names [4] = {'A', '1', '2', '3'};
    int i = 0;

    printf("Clock: %d\n\n", programClock);

    printf("REGISTERS\n---------------------------------\n");
    while (i < MAX_REGISTER)
    {
        printf("%1c\t", reg_names[i]);
        printHex(Registers[i]);
        printf("\n");
        ++i;
    }

    printf("PC\t");
    printHex(PC);
    printf("\n");

    printf("CC\t");
    printHex(CC);
    printf("\n");

    printf("IR\t");
    printHex(IR);

    printf("\n\nMEMORY\n---------------------------------\n");

    for (i = 0; i < MAX_MEMORY; i++)
    {
        if (memoryPrint[i] != 0)
        {
            printf("%-3d\t", i);
            printHex(mainMemory[i]);
            if (usedFrames[i / MEMORY_LENGTH] == 1)
                printf("LOCKED    USER %d", memoryPrint[i]);
            else if (usedFrames[i / MEMORY_LENGTH] == 0)
                printf("UNLOCKED");
            printf("\n");
        }
    }

    printf("\nDISK\n---------------------------------\n");

    for (i = 0; i < DISK_SIZE; i++)
    {
        if (diskPrint[i] != 0)
        {
            printf("%-3d\t", i);
            printHex(disk[i]);
            printf("\n");
        }
    }

    dumpQueues();

    printf("\n---------------------------------\n\tDUMP COMPLETE\n---------------------------------\n");
}

void stp()
{
    programClock++;
    queueArray[currentPriority][0]->isComplete = true;
    cleanUp();
    dmp();
    printf("\n\n---------------------------------\n\tMACHINE HALTED\n---------------------------------\n");
    exit(0);
}

void nop()
{
    programClock++;
    printf("No request added\n");
}

// Clean up
void cleanUp()
{
    printf("\n");
    int i = 0;

    if (queueArray[currentPriority][0]->isComplete == true)
        removeProcess(); // remove completed processes and shift queue
    else
    {
        rotateProcess(); // place unfinished processes at rear, shift queue
        promoteProcess();
    }
}

// ******************** MEMORY ********************

void mmu(int page, int frame)
{
    int pageNum;
    int offset;
    if (page == -1 && frame == -1 && opcode == 1)
    {
        pageNum = address & 252;
        pageNum = pageNum >> 2;
        offset = address & 3;
        address = (userArray[queueArray[currentPriority][0]->pid].pageTable[pageNum] * MEMORY_LENGTH) + offset;
    }
    else if (page > -1 && frame > -1)
    {
        userArray[queueArray[currentPriority][0]->pid].pageTable[page] = frame;
    }

    int i, j;
    if (page == -2 && frame == -2)
    {
        // Below is code to clean up users page table, zero's out their table (as they should only have one processes in queue)
        for (i = 0; i < MAX_MEMORY / MEMORY_LENGTH; i++)
        {
            for (j = 0; j < MAX_MEMORY / MEMORY_LENGTH; j++)
            {
                if (userArray[queueArray[currentPriority][0]->pid].pageTable[i] == j)
                {
                    userArray[queueArray[currentPriority][0]->pid].pageTable[i] = -1;
                    usedFrames[j] = 0;
                }
            }
        }
    }
}


// ******************** INTERPRETER ********************

// Fetches next instruction from mainMemory, then increments PC
void Fetch()
{
    IR = mainMemory[PC];
    PC++;
}

// Decode instructions into four fields: opcode, mode, register, address
void Decode()
{
    char temp[16];
    char *tempPointer = temp;

    unsigned int i = 1 << (sizeof(IR) * 8 - 1);

    int count = 0;
    int k = 0;

    while (i > 0)
    {
        if (IR & i)
            temp[k] = '1';
        else
            temp[k] = '0';
        i >>= 1;

        ++k;

        if (count == 3)
        {
            opcode = convertNumber(tempPointer);
            k = 0;
        }
        else if (count == 4)
        {
            if (temp[k - 1] == '0')
                mode = 0;
            else
                mode = 1;
            k = 0;
        }
        else if (count == 7)
        {
            temp[k] = 0;
            reg = convertNumber(tempPointer);
            k = 0;
        }
        else if (count == 15)
        {
            address = (short)convertNumber(tempPointer);
            k = 0;
        }

        ++count;
    }
}

// Based on opcode, execute the instruction
void Execute()
{
    switch (opcode)
    {
    case 0:  load(mainMemory, Registers);
        break;
    case 1:  store(mainMemory, Registers);
        break;
    case 2:  add(mainMemory, Registers);
        break;
    case 3:  sub(mainMemory, Registers);
        break;
    case 4:  adr(mainMemory, Registers);
        break;
    case 5:  sur(mainMemory, Registers);
        break;
    case 6:  and (mainMemory, Registers);
        break;
    case 7:  or (mainMemory, Registers);
        break;
    case 8:  not(mainMemory, Registers);
        break;
    case 9:  jmp(mainMemory);
        break;
    case 10: jeq(mainMemory);
        break;
    case 11: jgt(mainMemory);
        break;
    case 12: jlt(mainMemory);
        break;
    case 13: compare(mainMemory, Registers);
        break;
    case 14: clear(Registers);
        break;
    case 15: halt();
        break;
    }
    printf("\n");
    programClock++;
    currentTick++;
    queueArray[currentPriority][0]->frameTick++;
    queueArray[currentPriority][0]->executionTime++;
}


// ******************** FUNCTIONS ********************

// Called from halt instruction
void dumpPageTable()
{
    printf("\nUser %d Page Table\n", queueArray[currentPriority][0]->pid);
    int h, k;
    printf("Page \t| \tFrame\n");
    for (h = 0; h < (MAX_MEMORY / MEMORY_LENGTH); h++)
    {
        if (userArray[queueArray[currentPriority][0]->pid].pageTable[h] > -1)
        {
            printf("%d \t| \t%d\n", h, userArray[queueArray[currentPriority][0]->pid].pageTable[h]);
            for (k = 0; k < MEMORY_LENGTH; k++)
            {
                printf("\t\t%d:\t", userArray[queueArray[currentPriority][0]->pid].pageTable[h]*MEMORY_LENGTH + k);
                printHex(mainMemory[userArray[queueArray[currentPriority][0]->pid].pageTable[h]*MEMORY_LENGTH + k]);
                printf("\n");
            }
        }
    }
}

// Converts the string into an unsigned short
unsigned short convertNumber(char *num)
{
    return (unsigned short)strtoul(num, NULL, 2);
}

// Prints the passed integer in binary format
void printBin(unsigned short a)
{
    unsigned int i;
    i = 1 << (sizeof(a) * 8 - 1);
    int k = 0;

    while (i > 0)
    {
        if (a & i)
            printf("1");
        else
            printf("0");
        i >>= 1;
        ++k;
        if (k == 4)
        {
            printf(" ");
            k = 0;
        }
    }
}

// Prints the passed integer in hex format
void printHex(unsigned short a)
{
    printf("x%04X    ", a);
}

// Sets condition code of register to positive, zero, or negative
void changeCondition(int regValue)
{
    if (Registers[regValue] > 0) CC = 1;
    else if (Registers[regValue] == 0) CC = 2;
    else if (Registers[regValue] < 0) CC = 4;
}

bool isValidCommand()
{
    if (controlCommand[0] == 'r' && controlCommand[1] == 'u' && controlCommand[2] == 'n' && currentUser > 0)
    {
        commandCode = 1;
        return true;
    }
    else if (controlCommand[0] == 'd' && controlCommand[1] == 'm' && controlCommand[2] == 'p' && currentUser == 0)
    {
        commandCode = 2;
        return true;
    }
    else if (controlCommand[0] == 's' && controlCommand[1] == 't' && controlCommand[2] == 'p' && currentUser == 0)
    {
        commandCode = 3;
        return true;
    }
    else if (controlCommand[0] == 'n' && controlCommand[1] == 'o' && controlCommand[2] == 'p')
    {
        commandCode = 4;
        return true;
    }
    else
        return false;
}

void placeInQueue()
{
    int currentPosition; // Local variable for current position in the queue's
    if (commandCode == 3)
    {
        currentPosition = queueOneRear;
        queueOneRear++;
        currentPriority = 0;
        printf("Priority %d request added\n", currentPriority + 1);
    }
    else if (commandCode == 1)
    {
        currentPosition = queueTwoRear;
        queueTwoRear++;
        currentPriority = 1;
        printf("Priority %d request added\n", currentPriority + 1);
    }

    queueArray[currentPriority][currentPosition] = malloc(sizeof(processBlock));

    queueArray[currentPriority][currentPosition]->pid = currentUser;
    queueArray[currentPriority][currentPosition]->processType = commandCode;
    queueArray[currentPriority][currentPosition]->pageTick = 0;
    queueArray[currentPriority][currentPosition]->frameTick = 0;
    queueArray[currentPriority][currentPosition]->isRunning = false;
    queueArray[currentPriority][currentPosition]->executionTime = 0;
}

bool queueHasProcess(int priority)
{
    if (queueArray[priority][0] != NULL)
        return true;
    else
        return false;
}

void dumpQueues()
{
    int i;
    printf("\nPriority Queues\n---------------\nOne:   ");
    for (i = 0; i < numberOfProcesses; ++i)
    {
        if (queueArray[0][i] != NULL)
            printf("%d   ", queueArray[0][i]->pid);
        else
            printf("");
    }

    printf("\nTwo:   ");
    for (i = 0; i < numberOfProcesses; ++i)
    {
        if (queueArray[1][i] != NULL)
            printf("%d   ", queueArray[1][i]->pid);
        else
            printf("");
    }
    printf("\n");
}

void promoteProcess()
{
    int i;

    if (queueArray[1][0]->executionTime == 0)
    {
        printf("User %d process elevated to priority 1\n", queueArray[1][0]->pid);

        queueArray[0][queueOneRear] = queueArray[1][i];
        queueOneRear++;

        for (i = 0; i < queueTwoRear; i++)
            queueArray[1][i] = queueArray[1][i + 1];

        queueTwoRear--;
    }
}

void removeProcess()
{
    printf("User %d process removed from queue %d\n", queueArray[currentPriority][0]->pid, currentPriority + 1);

    int i;
    int rear;

    if (currentPriority == 0) rear = queueOneRear;
    if (currentPriority == 1) rear = queueTwoRear;

    for (i = 0; i < rear; i++)
        queueArray[currentPriority][i] = queueArray[currentPriority][i + 1];

    if (currentPriority == 0) queueOneRear--;
    if (currentPriority == 1) queueTwoRear--;

}

void rotateProcess()
{
    printf("User %d process moved to rear of queue %d\n", queueArray[currentPriority][0]->pid, currentPriority + 1);

    int i;
    int rear;

    if (currentPriority == 0) rear = queueOneRear;
    if (currentPriority == 1) rear = queueTwoRear;

    queueArray[currentPriority][rear] = queueArray[currentPriority][0];

    for (i = 0; i < rear + 1; i++)
        queueArray[currentPriority][i] = queueArray[currentPriority][i + 1];

}
