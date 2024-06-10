#include <stdio.h>

typedef unsigned char BYTE; // ex, 0x1F, 8 bits
typedef unsigned short int WORD; // ex, 0x1F24, 2bytes, 16 bits. size of words can vary, here it is 2 bytes

struct cpuContext
{
   BYTE m_Registers[16]; // 16 general purpose, single byte registers
   WORD m_AddressI; // special address reg - only last 12 bits needed
   WORD m_ProgramCounter; // PC, just use ++ to icnrement by 0x0001
   WORD m_StackPointer; // point to top of stack - some bullshit so who care, itll hold values 1-16
   WORD m_StackArr[16]; // arr of 16, 16bit values (stores address PC should ret to)

   BYTE m_GameMemory[0xFFF]; // just the memory
} context;

void reset(struct cpuContext *pContext) {
   pContext->m_ProgramCounter = 0x200; // the start of programs here
   //pContext->m_StackPointer = &(pContext->m_StackArr[0]); // pointer to first part of array
   pContext->m_StackPointer = 0; 

   // load in the game
   FILE *in;
   in = fopen( "INVADERS.ch8", "rb" );
   BYTE *mem_point = pContext->m_GameMemory;
   fread(&mem_point[0x200], 0xfff, 1, in);
   fclose(in);

}

int execute() {
   return 0;
}

WORD fetchOpcode(struct cpuContext *pContext) {
   WORD firstPart = pContext->m_GameMemory[pContext->m_ProgramCounter]; // say this is now 0x00A5
   firstPart = firstPart<<8;  // now 0xA500
   WORD secondPart = pContext->m_GameMemory[pContext->m_ProgramCounter+1]; // i.e. 0x00BE
   WORD finalOpcode = firstPart|secondPart; // 0xA5BE
   pContext->m_ProgramCounter += 2; 
   return finalOpcode;
}

// OPCODE SECTION
void op_0NNN(struct cpuContext *pContext, WORD opcode) {
   pContext->m_ProgramCounter = opcode & 0x0FFF;
}




int main() {
   printf("Hello, rld!");
   reset(&context);
   WORD fetchedOP = fetchOpcode(&context);
   //fetchedOP = fetchOpcode(&context);
   return 0;
}