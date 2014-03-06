/*     Authors: Austin Blythe, Matthew Francis
 *       Acct#: cs441102
 *       Class: CSC 441, Dr. Stader
 *  Start Date: 14 Jan 2013
 * Finish Date: 6 Feb 2013
 * Description: This program will simulate a machine language interpreter for a 
 *              specific architecture. 
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

   #define dataFileSize 8        // Hardcoded data file size
   #define MAX_MEMORY 256        // Size of memory array
   #define MAX_REGISTER 4        // Number of registers in the machine

// Global Varibles
   unsigned short CC = 0;        // Condition code
   unsigned short PC = 0;        // Program Counter
   unsigned short IR = 0;        // Instruction Register

   unsigned short opcode;        // Opcode field
   unsigned short mode;          // Mode field
   unsigned short reg;           // Register field
   signed short address;         // Address field

   int clock;
   unsigned short disk;

   // Internal
   int haltFlag = 0;             // Controls system halt
   char* dataFile[dataFileSize]; // Hardcoded data

   unsigned short InstrFile[MAX_MEMORY];  // Instructions/Data array
   signed short Registers[MAX_REGISTER];  // Registers array (0 is Accumulator)
   

// Method Declarations
   int main(void);

   // Functions
   void loadInstructions();
   void reset();
   unsigned short convertNumber(char*);
   void printBin(unsigned short);
   void printHex(unsigned short);
   void changeCondition(int);

   // Machine Instruction Cycle
   void Fetch();
   void Decode();
   void Execute();
   void Dump();

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
   
      char userIn[5] = {0};
      char* controlCommand = userIn;
      int stop = 0;
   

      // Primary data set
      dataFile[0]  = "080A"; // Load Immediate R0 #10
      dataFile[1]  = "1006"; // Store R0 6
      dataFile[2]  = "0905"; // Load Immediate R1 #5
      dataFile[3]  = "4100"; // AddR R1
      dataFile[4]  = "1007"; // Store R0 7
      dataFile[5]  = "F000"; // Halt
      dataFile[6]  = "0000"; // DATA
      dataFile[7]  = "0000"; // DATA

/*
      //Secondary data set
      dataFile[0]  = "080A"; // Load Immediate R0 #10
      dataFile[1]  = "0A05"; // Load Immediate R2 #5
      dataFile[2]  = "0B0F"; // Load Immediate R3 #15
      dataFile[3]  = "E000"; // Clear R0
      dataFile[4]  = "B006"; // JumpGT #6
      dataFile[5]  = "9009"; // Jump 7
      dataFile[6]  = "F000"; // HALT/JUNK
      dataFile[7]  = "2808"; // Add Immediate R0 #8
      dataFile[8]  = "3805"; // Sub Immediate R0 #5
      dataFile[9]  = "4200"; // AddR R2
      dataFile[10] = "5300"; // SubR R3
      dataFile[11] = "6800"; // And Immediate R0 #0
      dataFile[12] = "78F0"; // Or Immediate R0 xF0
      dataFile[13] = "8000"; // Not R0
      dataFile[14] = "1014"; // Store R0 #20
      dataFile[15] = "9013"; // Jump 19
      dataFile[16] = "F000"; // Junk
      dataFile[17] = "F000"; // Junk
      dataFile[18] = "F000"; // Junk
      dataFile[19] = "F000"; // Halt
      dataFile[20] = "0000"; // DATA
*/

      while(stop == 0) {
         printf("\nPlease enter a command: ");
         fgets(controlCommand, 5, stdin);
         printf("\n");

         if(controlCommand[0] == 'r' && controlCommand[1] == 'u' && controlCommand[2] == 'n'){

            reset();
            loadInstructions();

            printf("Running...\n");
            while(haltFlag == 0) {
               Fetch();   
               Decode();
               Execute();
            }
            printf("Execution complete.\n");
         }
      	
         if(controlCommand[0] == 'd' && controlCommand[1] == 'm' && controlCommand[2] == 'p'){
         	Dump();
         }
      	
         if(controlCommand[0] == 'n' && controlCommand[1] == 'o' && controlCommand[2] == 'p'){
           printf("\tNo operation performed.\n");
         }
      	
         if(controlCommand[0] == 's' && controlCommand[1] == 't' && controlCommand[2] == 'p'){
            stop = 1;
            Dump();
            printf("\n\tMachine halted.\n\n");
         }
      
      }

   
      return 0;
   }


// ******************** FUNCTIONS ********************

   // Loads instructions from data source(file, hardcode, etc)
   void loadInstructions() {
   
      int i = 0;
      for (; i < 
      Size; i++) {
         InstrFile[i] = (unsigned short)strtoul(dataFile[i], NULL, 16);
      }
   }

   // This method will zero out all of the registers
   void reset() {
   
      int i = 0;
      while (i < MAX_REGISTER) {
         Registers[i] = 0;
         ++i;
      }

      PC = 0;
      IR = 0;
      CC = 0;
      haltFlag = 0;
   }

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


// ******************** MACHINE INSTRUCTION CYCLE ********************

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
   }

   // This will create a dump of the data in the program
   void Dump() {
   
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
      printf("\nMEMORY\n---------------------------------\n");

      for (i = 0; i < MAX_MEMORY; i++) {
         printf("%-3d  ", i);
         printHex(InstrFile[i]);
         printBin(InstrFile[i]);
      }
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
      haltFlag = 1;
      printf("\tHalt\n");
   }
