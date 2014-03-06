/*     Authors: Austin Blythe, Matthew Francis
 *       Acct#: cs441102
 *       Class: CSC 441, Dr. Stader
 *  Start Date: 6 Feb 2013
 * Finish Date: 20 Feb 2013
 * Description: This program builds multi-user and mutual exclusion functionality
 *              into the machine language interpreter of OS Part 1.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
   
// Global Constants
   const int OPCODE_LEN = 4;     // Opcode field length
   const int MODE_LEN = 1;       // Mode field length
   const int REG_LEN = 3;        // Register field length
   const int ADDRE_LEN = 8;      // Address field length
   const int MAX_BITS = 16;      // Size of a single memory location in bits
   const int TICKS_PER_USER = 4; // # of Ticks allowed per cycle for user

   #define MAX_MEMORY 256        // Size of memory array
   #define MAX_REGISTER 4        // Number of registers in the machine
   #define DISK_SIZE 256         // Size of the disk
   #define true 1		            // Setting keyword true to 1
   #define false 0               // Setting keyword false to 0
   #define numberOfUsers 3  		// Number of users in the system
   #define numberOfProcesses 10  // Number of simultaneous processes (possible) in the system
	
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
   bool memLocked;               // Global memory lock
   bool diskLocked;              // Disk lock    
   int clock;                    // Internal Clock
   bool stopOS;                  // Changes to true if stp is issues
   int currentTick;              // Current tick that the user is on (resets to 0 when limit reached)
   int currentUser;              // Current user in the RR

   // Structure format creation for users and O/S
   struct user{
      int memoryLocation;        // Starting location for that users program
      bool hasProcess;           // If true, user has a process in the queue
   };  
   struct user userArray[numberOfUsers];  // Array for creation of users + 1 to allow extra spot

   struct processBlock {
      int pid;
      unsigned short pCounter;
      bool isRunning;
      bool isComplete;
   };

   struct semaphore {
      int count;
   } semaphore;

   struct processBlock processArray[numberOfProcesses];

   unsigned short disk[DISK_SIZE];  // 1D-array of short int
   unsigned short InstrFile[MAX_MEMORY];  // Instructions/Data array
   signed short Registers[MAX_REGISTER];  // Registers array (0 is Accumulator)
   
   char userIn[5] = {0};                  // Array for user input
   char* controlCommand = userIn;         // Pointer to userIn array

// Method Declarations
   int main(void);

   // Operating System
   void initializeOS();
   void scheduler();
   void dispatcher();
   void userInterface();
   void interpreter();
   bool userHasProcess();                // Checks if the user has a process in the queue
   void cleanUp();
   void Dump();

   // UI Commands
   void run();
   void nop();
   void dmp();
   void stp();

   // Interpreter
   void Fetch();
   void Decode();
   void Execute();
   
   // Functions
   unsigned short convertNumber(char*);
   void printBin(unsigned short);
   void printHex(unsigned short);
   void changeCondition(int);

   // Instruction Set
   void load();
   void store();
   void add();
   void sub();
   void adr();
   void sur();
   void and();
   void or();
   void not();
   void jmp();
   void jeq();
   void jgt();
   void jlt();
   void compare();
   void clear();
   void halt();




// ******************** MAIN ********************

   int main (void) {
   
      // OS Initialization
      initializeOS();   
      
      // Round robin scheduler (user1, user2, o/s)
      scheduler();
   
      return 0;
   }


// ******************** OPERATING SYSTEM ********************

   void initializeOS(){
   // Initialize Values
      clock = 0;
      CC = 0;
      PC = 0;
      IR = 0;
      haltFlag = false;
      memLocked = false;
      diskLocked = false;
      stopOS = false;
      currentUser = 1;  // Starts with user 1
      currentTick = 0;  // User 1 starts with 0 ticks on their cycle
   
   // Zero out the InstrFile
      int p = 0;
      for(p; p < MAX_MEMORY; p++){
         InstrFile[p] = 0x0000;
      }
   
   // Load user programs into memory
         // User 1 data set
      InstrFile[0]   = 0x080A; // Location 000 // Load Immediate R0 #10
      InstrFile[1]   = 0x1006; // Location 001 // Store R0 6
      InstrFile[2]   = 0x0905; // Location 002 // Load Immediate R1 #5
      InstrFile[3]   = 0x4100; // Location 003 // AddR R1
      InstrFile[4]   = 0x1007; // Location 004 // Store R0 7
      InstrFile[5]   = 0xF000; // Location 005 // Halt
         
         // User 2 data set
      InstrFile[100] = 0x0819; // Location 100 // LOAD I R0 #25
      InstrFile[101] = 0x106A; // Location 101 // STO R0 106
      InstrFile[102] = 0x0905; // Location 102 // LOD I R1 #5
      InstrFile[103] = 0x5100; // Location 103 // SUR R1
      InstrFile[104] = 0x1007; // Location 104 // STO R0 7
      InstrFile[105] = 0xF000; // Location 105 // HALT
   
   // Copy memory to the disk (for reinitializing memory)
      for (p = 0; p < MAX_MEMORY; p++) {
         disk[p] = InstrFile[p];
      }
   
   // Create user(s)
         // OS
      userArray[0].memoryLocation = 0;
   
         // User1
      userArray[1].memoryLocation = 0;
   
         // User2
      userArray[2].memoryLocation = 100;
   
      semaphore.count = 0;
   }

   void scheduler() {
      while (stopOS == false) {
         currentTick = 0;
      
         dispatcher();
         
         if (currentUser == 2) {
            currentUser = 0;
         }
         else {
            currentUser++;
         }
      }   
   }

   // Directs users/OS based on command entered and semaphore status
   void dispatcher() {
      int validCommand = false;
      
      if (processArray[0].pid == currentUser && currentUser > 0) {
         PC = processArray[0].pCounter;
         interpreter();
      }
   
      if(currentTick < TICKS_PER_USER) {
         while (!validCommand) {
            userInterface();
            if(controlCommand[0] == 'r' && controlCommand[1] == 'u' && controlCommand[2] == 'n'){
               if(currentUser > 0 && userArray[currentUser].hasProcess == false) {
                  processArray[semaphore.count].pid = currentUser;
                  processArray[semaphore.count].pCounter = userArray[currentUser].memoryLocation;
                  semaphore.count++;
                  userArray[currentUser].hasProcess = true;
                  clock++;
                  currentTick++;
                  if (processArray[0].pid == currentUser) {
                     PC = userArray[currentUser].memoryLocation;
                     processArray[0].isRunning = true;
                     memLocked = true;
                     interpreter();
                  } 
                  else {
                     printf("Memory is locked. Your process has been added to the queue.\n");
                  }
                  validCommand = true;
               }
               else if (currentUser > 0 && userArray[currentUser].hasProcess == true) {
                  printf("Your process is already queued. Please wait.\n");
               } 
               else {
                  printf("You are not authorized to issue that command.\n");
               }
            }
            
            else if(controlCommand[0] == 'd' && controlCommand[1] == 'm' && controlCommand[2] == 'p'){
               if( currentUser == 0) {
                  Dump();
                  validCommand = true;
               }
               else {
                  printf("You are not authorized to issue that command.\n");
               }
               clock++;
               currentTick++;
            }
            
            else if(controlCommand[0] == 'n' && controlCommand[1] == 'o' && controlCommand[2] == 'p'){
               printf("\tNo operation performed.\n");
               validCommand = true;
               clock++;
               currentTick++;
            }
            
            else if(controlCommand[0] == 's' && controlCommand[1] == 't' && controlCommand[2] == 'p'){
               if(currentUser == 0){ 
                  stopOS = true;
                  Dump();
                  printf("\n\tMachine halted.\n\n");
               }
               else {
                  printf("You are not authorized to issue that command.\n");
               }
               validCommand = true;
               clock++;
               currentTick++;
            
            }
            
            else {
               printf("\tInvalid command entered\n");
            }
         }
      }
   }

   // Interactive command-line user interface
   void userInterface() {
      if (currentUser == 0) printf("\n\tO/S");
      else printf("\n\tUser %i", currentUser);
//      printf("\nCurrent tick: %d", currentTick);
      printf("\nPlease enter a command: ");
   
      fgets(controlCommand, 5, stdin);
      printf("\n");
   }

   // responsible for the machine languare interpretation and execution
   void interpreter() {
      if (currentUser > 0) printf("\nUser %d Running...\n", currentUser);
      processArray[0].isRunning = true;
      memLocked = true;
      while(haltFlag == false && currentTick < TICKS_PER_USER) {
         Fetch();   
         Decode();
         Execute();
      }
   
      processArray[0].pCounter = PC;
   
      if (haltFlag == true) {
         processArray[0].isComplete = true;
      }
      cleanUp();
      haltFlag = false; // reset halt flag for subsequent program runs
   }

   // Clean up
   void cleanUp() {
      int i;
      if (processArray[0].isComplete == true) {
         processArray[0].isRunning = false;
         memLocked = false;
         semaphore.count--;
         userArray[currentUser].hasProcess = false;
      
         for (i = 0; i < numberOfProcesses - 1; i++) {
            processArray[i].pid = processArray[i+1].pid;
            processArray[i].pCounter = processArray[i+1].pCounter;
            processArray[i].isComplete = processArray[i+1].isComplete;
            processArray[i].isRunning = processArray[i+1].isRunning;
         }
         processArray[numberOfProcesses - 1].pid = 0;
         processArray[numberOfProcesses - 1].pCounter = 0;
         processArray[numberOfProcesses - 1].isComplete = false;
         processArray[numberOfProcesses - 1].isRunning = false;
         
      }  
   }

   // This will create a dump of the data in the program
   void Dump() {
      clock++;
      currentTick++;
      
      char reg_names [4] = {'A', '1', '2', '3'};
      int i = 0;
    
      printf("REGISTERS\n---------------------------------\n");
      while(i < MAX_REGISTER) {
         printf("%1c    ", reg_names[i]);
         printHex(Registers[i]);
         printBin(Registers[i]);
         ++i;
      }
   
      printf("PC   ");
      printHex(PC);
      printBin(PC);
   
      printf("CC   ");
      printHex(CC);
      printBin(CC);
   
      printf("IR   ");
      printHex(IR);
      printBin(IR);
   
      printf("Clock: %d\n", clock);
   
      printf("\nMEMORY\n---------------------------------\n");
   
      for (i = 0; i < MAX_MEMORY; i++) {
         if(InstrFile[i] != 0x0000){
            printf("%-3d  ", i);
            printHex(InstrFile[i]);
            printBin(InstrFile[i]);
         }
      }
   
      printf("\nDISK\n---------------------------------\n");
   
      for (i = 0; i < DISK_SIZE; i++) {
         if(disk[i] != 0x0000){
            printf("%-3d  ", i);
            printHex(disk[i]);
            printBin(disk[i]);
         }
      }
   
      if (memLocked) printf("\nMEMORY LOCK STATUS: Locked\n");
      else printf("\nMEMORY LOCK STATUS: Unlocked\n");



      printf("\nPROCESS QUEUE\n---------------------------------\n");
   
      if (processArray[0].pid > 0) {
         printf("Process Owner\tMem Location\tLocked Memory\t\n");
         for (i = 0; i < numberOfProcesses; i++) {
            if (processArray[i].pid != 0) {
               printf("\t%d\t\t%d\t\t", processArray[i].pid, processArray[i].pCounter);
               if (processArray[i].isRunning == true) {
                  printf("*\n");
               } 
               else {
                  printf("-\n");
               }
            }
         }
      } 
      else {
         printf("Process queue empty\n");
      }
   
      printf("\n---------------------------------\nDUMP COMPLETE\n---------------------------------\n");

   }
   
// ******************** UI ****************
	


// ******************** INTERPRETER ********************

   // Fetches next instruction from InstrFile, then increments PC
   void Fetch() {
      IR = InstrFile[PC];
      PC++;
   }

   // Decode instructions into four fields: opcode, mode, register, address
   void Decode() {
   
      char temp[16];
      char* tempPointer = temp;
   
      unsigned int i = 1<<(sizeof(IR) * 8-1);
   
      int count = 0;
      int k = 0;
   
      while(i > 0){
         if(IR & i)
            temp[k] = '1';
         else
            temp[k] = '0';
         i >>= 1;
      
         ++k;
      
         if(count == 3){
            opcode = convertNumber(tempPointer);
            k = 0;
         }
         else if(count == 4){
            if(temp[k-1] == '0')
               mode = 0;
            else
               mode = 1;
            k = 0;
         }
         else if(count == 7){
            temp[k] = 0;
            reg = convertNumber(tempPointer);
            k = 0;  
         }
         else if(count == 15){
            address = (short)convertNumber(tempPointer);
            k = 0;  
         }
      
         ++count;
      }
   }

   // Based on opcode, execute the instruction
   void Execute() {
      switch (opcode) {
         case 0:     load(InstrFile, Registers); 
            break;
         case 1:     store(InstrFile, Registers); 
            break;
         case 2:     add(InstrFile, Registers); 
            break;
         case 3:     sub(InstrFile, Registers); 
            break;
         case 4:     adr(InstrFile, Registers); 
            break;
         case 5:     sur(InstrFile, Registers); 
            break;
         case 6:     and(InstrFile, Registers); 
            break;
         case 7:     or(InstrFile, Registers); 
            break;
         case 8:     not(InstrFile, Registers); 
            break;
         case 9:     jmp(InstrFile); 
            break;
         case 10:    jeq(InstrFile); 
            break;
         case 11:    jgt(InstrFile); 
            break;
         case 12:    jlt(InstrFile); 
            break;
         case 13:    compare(InstrFile, Registers); 
            break;
         case 14:    clear(Registers); 
            break;
         case 15:    halt(); 
            break;
         default:    
            break;
      }
      clock++;
      currentTick++;
   }


// ******************** FUNCTIONS ********************

   // Converts the string into an unsigned short
   unsigned short convertNumber(char* num){
      return (unsigned short)strtoul(num, NULL, 2);
   }

   // Prints the passed integer in binary format
   void printBin(unsigned short a) {
   
      unsigned int i;
      i = 1<<(sizeof(a) * 8-1);
      int k = 0;
   
      while(i > 0) {
         if(a & i)
            printf("1");
         else
            printf("0");
         i >>= 1;
         ++k;
         if(k == 4){
            printf(" ");
            k = 0;
         }
      }
      printf("\n");
   }

   // Prints the passed integer in hex format
   void printHex(unsigned short a) {
      printf("x%04X    ", a);
   }

   // Sets condition code of register to positive, zero, or negative
   void changeCondition(int regValue) {
      if (Registers[regValue] > 0) CC = 1;
      else if (Registers[regValue] == 0) CC = 2;
      else if (Registers[regValue] < 0) CC = 4;
      else {}
   }


// ******************** INSTRUCTIONS ********************  

   void load() {
      printf("\tLoad\n");
      if (mode == 1) {
         Registers[reg] = address;
      } 
      else {
         Registers[reg] = (short)InstrFile[address];
      }
      
      changeCondition(reg);
   }

   void store() {
      printf("\tStore\n");
      InstrFile[address] = (unsigned short)Registers[reg];
   
      changeCondition(reg);
   }

   void add() {
      printf("\tAdd\n");
      if (mode == 1) {
         Registers[reg] = Registers[0] + address;
      } 
      else {
         Registers[reg] = Registers[0] + (short)InstrFile[address];
      }
      
      changeCondition(reg);
   }

   void sub() {
      printf("\tSubtract\n");
      if (mode == 1) {
         Registers[reg] = Registers[0] - address;
      } 
      else {
         Registers[reg] = Registers[0] - (short)InstrFile[address];
      }
      
      changeCondition(reg);
   }
   
   void adr() {
      printf("\tAdd Register\n");
      Registers[0] = Registers[0] + Registers[reg];
   
      changeCondition(0);
   }

   void sur() {
      printf("\tSubtract Register\n");
      Registers[0] = Registers[0] - Registers[reg];
   
      changeCondition(0);
   }
   
   void and() {
      printf("\tAnd\n");
      if (mode == 1) {
         Registers[reg] = Registers[0] & address;
      } 
      else {
         Registers[reg] = Registers[0] & (short)InstrFile[address];
      }
    
      changeCondition(reg);
   }

   void or() {
      printf("\tOr\n");
      if (mode == 1) {
         Registers[reg] = Registers[0] | address;
      } 
      else {
         Registers[reg] = Registers[0] | (short)InstrFile[address];
      }
   
      changeCondition(reg);
   }

   void not() {
      printf("\tNot\n");
   
      Registers[reg] = ~Registers[reg];
    
      changeCondition(reg);
   }

   void jmp() {
      printf("\tJump\n");
      PC = (unsigned short)address;
   }

   void jeq() {
      printf("\tJump Equal\n");
      if (CC == 2) PC = (unsigned short)address;
   }

   void jgt() {
      printf("\tJump Greater\n");
      if (CC == 1) PC = (unsigned short)address;
   }

   void jlt() {
      printf("\tJump Less\n");
      if (CC == 4) PC = (unsigned short)address;
   }

   void compare() {
      printf("\tCompare\n");
      if (Registers[reg] > 0) {
         CC = 1;
      } 
      else if(Registers[reg] == 0) {
         CC = 2;
      } 
      else if(Registers[reg] < 0) {
         CC = 4;
      } 
      else {}
   }

   void clear() {
      printf("\tClear\n");
      Registers[reg] = 0;
   
      changeCondition(reg);
   }

   void halt() {
      haltFlag = true;
      printf("\tHalt\n");
   
      printf("Execution complete.\n");
   }
