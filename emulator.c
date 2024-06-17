#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "blah.h"

// TODO - fix the VF to actually be VF

typedef unsigned char BYTE; // ex, 0x1F, 8 bits
typedef unsigned short int WORD; // ex, 0x1F24, 2bytes, 16 bits. size of words can vary, here it is 2 bytes

struct cpuContext
{
   BYTE m_Registers[16]; // 16 general purpose, single byte registers
   BYTE m_VF; // special reg
   WORD m_AddressI; // special address reg - only last 12 bits needed
   WORD m_ProgramCounter; // PC, just use ++ to icnrement by 0x0001
   WORD m_StackPointer; // point to top of stack - some bullshit so who care, itll hold values 0-15. If its at 15, its full, no more calls
   WORD m_StackArr[16]; // arr of 16, 16bit values (stores address PC should ret to)
   BYTE m_ScreenData[32][64] ;

   BYTE m_GameMemory[0xFFF]; // just the memory
} context;

void reset(struct cpuContext *pContext) {
   pContext->m_ProgramCounter = 0x200; // the start of programs here
   //pContext->m_StackPointer = &(pContext->m_StackArr[0]); // pointer to first part of array
   pContext->m_StackPointer = 0; 
   
   // do i need to set regs?
   pContext->m_VF = 0;

   // load in the game
   FILE *in;
   in = fopen( "optest.ch8", "rb" );
   BYTE *mem_point = pContext->m_GameMemory;
   fread(&mem_point[0x200], 0xfff, 1, in);
   fclose(in);

}

// OPCODE SECTION

// Jump to a machine code routine at nnn.
void op_0NNN(struct cpuContext *pContext, WORD opcode) {
   pContext->m_ProgramCounter = opcode & 0x0FFF;
}

// Clear the display.
void op_00E0(struct cpuContext *pContext, WORD opcode) {
   for (int r = 0; r < 32; r++) {
      for (int c = 0; c < 64; c++) {
         pContext->m_ScreenData[r][c] = 0;
      }
   }
   //memset(pContext->m_ScreenData[r][c], 0, sizeof(pContext->m_ScreenData));
}

// Return from a subroutine.
void op_00EE(struct cpuContext *pContext, WORD opcode) {
   //pContext->m_StackPointer = pContext->m_StackPointer-1;
   pContext->m_ProgramCounter = pContext->m_StackArr[--pContext->m_StackPointer];
}

// Jump to location nnn.
void op_1NNN(struct cpuContext *pContext, WORD opcode) {
   pContext->m_ProgramCounter = opcode & 0x0FFF;
}

// Call subroutine at nnn.
void op_2NNN(struct cpuContext *pContext, WORD opcode) {
   pContext->m_StackArr[pContext->m_StackPointer++] = pContext->m_ProgramCounter; // set stackArr[oldSP]=PC, increment oldSP
   pContext->m_ProgramCounter = opcode & 0x0FFF;
}

// Skip next instruction if Vx = kk.
void op_3xkk(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber];
   int imm = opcode & 0x00FF;
   if (imm == regContents) { // if Vx = kk
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// Skip next instruction if Vx != kk.
void op_4xkk(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber];
   int imm = opcode & 0x00FF;
   if (imm != regContents) { // if Vx != kk
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// Skip next instruction if Vx = Vy.
void op_5xy0(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX];
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   BYTE regContentsY = pContext->m_Registers[regNumY];

   if (regContentsX == regContentsY) { // if Vx = kk
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// Set Vx = kk.
void op_6xkk(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_Registers[regNumX] = (opcode & 0x00FF);
}

// Set Vx = Vx + kk.
void op_7xkk(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_Registers[regNumX] += (opcode & 0x00FF);
}

// Set Vx = Vy.
void op_8xy0(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   pContext->m_Registers[regNumX] = pContext->m_Registers[regNumY];
}

// Set Vx = Vx OR Vy.
void op_8xy1(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   pContext->m_Registers[regNumX] = pContext->m_Registers[regNumX] | pContext->m_Registers[regNumY];
}

// Set Vx = Vx AND Vy.
void op_8xy2(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   pContext->m_Registers[regNumX] = pContext->m_Registers[regNumX] & pContext->m_Registers[regNumY];
}

// Set Vx = Vx XOR Vy.
void op_8xy3(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   pContext->m_Registers[regNumX] = pContext->m_Registers[regNumX] ^ pContext->m_Registers[regNumY];
}

// Set Vx = Vx + Vy, set VF = carry.
void op_8xy4(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   
   WORD regContentsX = pContext->m_Registers[regNumX]; // PROMOTE these guys from 0x00, to 0x0000
   WORD regContentsY = pContext->m_Registers[regNumY];
   if (((regContentsX+regContentsY) >> 8) != 0x0000) { // if it is like 0x0100, do carry
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }
   pContext->m_Registers[regNumX] = pContext->m_Registers[regNumX] + pContext->m_Registers[regNumY];
}

// Set Vx = Vx - Vy, set VF = NOT borrow.
void op_8xy5(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F

   BYTE regContentsX = pContext->m_Registers[regNumX]; // underflow is intentional if happens
   BYTE regContentsY = pContext->m_Registers[regNumY];

   if (regContentsX > regContentsY) {
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }

   pContext->m_Registers[regNumX] = regContentsX - regContentsY;
}

// Set Vx = Vx SHR 1.
void op_8xy6(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX]; 
   if (regContentsX & 0x01 == 0x01) { // if LSB is 1, set flag
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }
   pContext->m_Registers[regNumX] = regContentsX/2;
}

// SUBN: Set Vx = Vy - Vx, set VF = NOT borrow
void op_8xy7(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F

   BYTE regContentsX = pContext->m_Registers[regNumX]; // underflow is intentional if happens
   BYTE regContentsY = pContext->m_Registers[regNumY];

   if (regContentsY > regContentsX) {
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }

   pContext->m_Registers[regNumX] = regContentsY - regContentsX;
}

// SHL: Set Vx = Vx SHL 1.
void op_8xyE(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX]; 
   if (regContentsX & 0x80 == 0x80) { // if MSB is 1, set flag. 0x80 is 1000 0000
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }
   pContext->m_Registers[regNumX] = regContentsX*2;
}

// SNE: Skip next instruction if Vx != Vy.
void op_9xy0(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX];
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   BYTE regContentsY = pContext->m_Registers[regNumY];

   if (regContentsX != regContentsY) { // if Vx != Vy
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// LD: Set I = nnn.
void op_Annn(struct cpuContext *pContext, WORD opcode) {
   pContext->m_AddressI = opcode & 0x0FFF;
}

// JP: Jump to location nnn + V0.
void op_Bnnn(struct cpuContext *pContext, WORD opcode) {
   pContext->m_ProgramCounter = (opcode & 0x0FFF) + pContext->m_Registers[0];
}

// FIX THIS
// RND: Set Vx = random byte AND kk. 
void op_Cxkk(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_Registers[regNumX] = 0x20 & (opcode & 0x00FF); // TODO: use 32 as placeholder for random
}

// DRW: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
void op_Dxyn(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int regNumY = (opcode & 0x00F0) >> 4; // shift right one place -> from 0x00F0 to 0x000F
   
   BYTE startX = pContext->m_Registers[regNumX];
   BYTE startY = pContext->m_Registers[regNumY];

   int numBytes = (opcode & 0x000F);

   int XORpixel = 0;
   for (int i = 0; i < numBytes; i++) {
      BYTE byteData = pContext->m_GameMemory[pContext->m_AddressI+i];

      for (int j = 0; j < 8; j++) { // go through the byte's bit and XOR onto display
         int drawPosX = (startX+j);//%64;
         int drawPosY = (startY+i);//%32;

         int byteDataBit = byteData & (0x01 << (7-j)); // this lets us get proper mask; e.g., b0100.0000 is "second" pixel, j = 1, so << by 6
         if (byteDataBit & byteData) { // if it not 0, then we toggle the onscren position
            if (pContext->m_ScreenData[drawPosY][drawPosX] == 1) { // pixel is already onscreen, we must erase pixel
               pContext->m_ScreenData[drawPosY][drawPosX] = 0;
               XORpixel = 1;
            } else { // pixel is not onscreen, color it in
               pContext->m_ScreenData[drawPosY][drawPosX] = 1;
            }
         }
      }
   }
   if (XORpixel == 1) {
      pContext->m_VF = 1;
   } else {
      pContext->m_VF = 0;
   }
}

WORD fetchOpcode(struct cpuContext *pContext) {
   WORD firstPart = pContext->m_GameMemory[pContext->m_ProgramCounter]; // say this is now 0x00A5
   firstPart = firstPart<<8;  // now 0xA500
   WORD secondPart = pContext->m_GameMemory[pContext->m_ProgramCounter+1]; // i.e. 0x00BE
   WORD finalOpcode = firstPart|secondPart; // 0xA5BE
   pContext->m_ProgramCounter += 2; // by 2 -> remember, 1byte addressability, so each opcode 2bytes
   return finalOpcode;
}

void executeOpcode(struct cpuContext *pContext, WORD opcode) {
   switch (opcode & 0xF000) {
      case 0x0000:
         switch(opcode & 0x000F) {
            case 0x0000: op_00E0(pContext, opcode); break; // clear 
            case 0x000E: op_00EE(pContext, opcode); break; // ret
         } break;
      case 0x1000: op_1NNN(pContext, opcode); break; // jump
      case 0x2000: op_2NNN(pContext, opcode); break; // call
      case 0x3000: op_3xkk(pContext, opcode); break; // se
      case 0x4000: op_4xkk(pContext, opcode); break; // sne
      case 0x5000: op_5xy0(pContext, opcode); break; // se
      case 0x6000: op_6xkk(pContext, opcode); break; // ld
      case 0x7000: op_7xkk(pContext, opcode); break; // add
      case 0x8000:
         switch(opcode & 0x000F) {
            case 0x0000: op_8xy0(pContext, opcode); break; // ld
            case 0x0001: op_8xy1(pContext, opcode); break; // or
            case 0x0002: op_8xy2(pContext, opcode); break; // and
            case 0x0003: op_8xy3(pContext, opcode); break; // xor
            case 0x0004: op_8xy4(pContext, opcode); break; // add
            case 0x0005: op_8xy5(pContext, opcode); break; // sub
            case 0x0006: op_8xy6(pContext, opcode); break; // shr
            case 0x0007: op_8xy7(pContext, opcode); break; // subn
            case 0x000E: op_8xyE(pContext, opcode); break; // shl
         } break;
      case 0x9000: op_9xy0(pContext, opcode); break; // sne
      case 0xA000: op_Annn(pContext, opcode); break; // ld
      case 0xB000: op_Bnnn(pContext, opcode); break; // jp
      case 0xC000: op_Cxkk(pContext, opcode); break; // rand
      case 0xD000: op_Dxyn(pContext, opcode); break; // draw
      default: printf("ERROR: UNIMPLEMENTED OPCODE"); break;
   }
}



void test_op_3xkk(struct cpuContext *pContext) {
   reset(pContext);
   pContext->m_Registers[4] = 0x3A; // x
   pContext->m_GameMemory[pContext->m_ProgramCounter] = 0x34; // kk is 0x343A
   pContext->m_GameMemory[pContext->m_ProgramCounter+1] = 0x3A; 
   WORD opcode = fetchOpcode(pContext);
   int initPC = pContext->m_ProgramCounter;
   executeOpcode(pContext, opcode);
   if (initPC == pContext->m_ProgramCounter) { // they were same -> so it should have increment PC, thus be unequal
      printf("TEST FAILED: OP 3xkk");
   } else {
      printf("TEST SUCCEEDED: OP 3xkk");
   }
}




int main() {
   printf("Hello, rld!");
   drawWindow();
   reset(&context);
   while (1 == 1) {
      WORD fetchedOP = fetchOpcode(&context);
      executeOpcode(&context, fetchedOP);
      drawPixels(context.m_ScreenData);
      //sleep(1);
   }
   //test_op_3xkk(&context);


   
   
   //fetchedOP = fetchOpcode(&context);
   drawWindow();
   /** 
   for (int i = 0; i < 10; i++) {
      drawPixel(i, 2, 0);
   }
   for (int i = 0; i < 10; i++) {
      drawPixel(i, 2, 1);
   }
   */
  for (int i = 0; i < 10; i++) {
      context.m_ScreenData[i][10] = 1; // draw row 1-10, column 10; vert line
   }
   drawPixels(context.m_ScreenData);
   //op_00E0(&context, fetchedOP);
   drawPixels(context.m_ScreenData);
   //drawRect();
   return 0;
}