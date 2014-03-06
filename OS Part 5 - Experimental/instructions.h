

// ******************** INSTRUCTIONS ********************  

   void load() {
      printf("\tLoad");
      if (mode == 1) {
         Registers[reg] = address;
      } 
      else {
         Registers[reg] = (short)mainMemory[address];
      }
      
      changeCondition(reg);
   }

   void store() {
      printf("\tStore");
      mainMemory[address] = (unsigned short)Registers[reg];
   
      changeCondition(reg);
   }

   void add() {
      printf("\tAdd");
      if (mode == 1) {
         Registers[reg] = Registers[0] + address;
      } 
      else {
         Registers[reg] = Registers[0] + (short)mainMemory[address];
      }
      
      changeCondition(reg);
   }

   void sub() {
      printf("\tSubtract");
      if (mode == 1) {
         Registers[reg] = Registers[0] - address;
      } 
      else {
         Registers[reg] = Registers[0] - (short)mainMemory[address];
      }
      
      changeCondition(reg);
   }
   
   void adr() {
      printf("\tAdd Register");
      Registers[0] = Registers[0] + Registers[reg];
   
      changeCondition(0);
   }

   void sur() {
      printf("\tSubtract Register");
      Registers[0] = Registers[0] - Registers[reg];
   
      changeCondition(0);
   }
   
   void and() {
      printf("\tAnd");
      if (mode == 1) {
         Registers[reg] = Registers[0] & address;
      } 
      else {
         Registers[reg] = Registers[0] & (short)mainMemory[address];
      }
    
      changeCondition(reg);
   }

   void or() {
      printf("\tOr");
      if (mode == 1) {
         Registers[reg] = Registers[0] | address;
      } 
      else {
         Registers[reg] = Registers[0] | (short)mainMemory[address];
      }
   
      changeCondition(reg);
   }

   void not() {
      printf("\tNot");
   
      Registers[reg] = ~Registers[reg];
    
      changeCondition(reg);
   }

   void jmp() {
      printf("\tJump");
      PC = (unsigned short)address;
   }

   void jeq() {
      printf("\tJump Equal");
      if (CC == 2) PC = (unsigned short)address;
   }

   void jgt() {
      printf("\tJump Greater");
      if (CC == 1) PC = (unsigned short)address;
   }

   void jlt() {
      printf("\tJump Less");
      if (CC == 4) PC = (unsigned short)address;
   }

   void compare() {
      printf("\tCompare");
      if (Registers[reg] > 0) {
         CC = 1;
      } 
      else if(Registers[reg] == 0) {
         CC = 2;
      } 
      else if(Registers[reg] < 0) {
         CC = 4;
      } 
   }

   void clear() { 
      printf("\tClear");
      Registers[reg] = 0;
   
      changeCondition(reg);
   }

   void halt() {
      haltFlag = true;
      printf("\tHalt\n");
      
      printf("Execution complete.");
   }
