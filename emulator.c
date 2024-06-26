#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "blah.h"

// TODO - fix the VF to actually be VF

typedef unsigned char BYTE; // ex, 0x1F, 8 bits
typedef unsigned short int WORD; // ex, 0x1F24, 2bytes, 16 bits. size of words can vary, here it is 2 bytes

struct cpuContext
{
   BYTE m_Registers[16]; // 16 general purpose, single byte registers
   WORD m_AddressI; // special address reg - only last 12 bits needed
   WORD m_ProgramCounter; // PC, just use ++ to icnrement by 0x0001
   WORD m_StackPointer; // point to top of stack - some bullshit so who care, itll hold values 0-15. If its at 15, its full, no more calls
   WORD m_StackArr[16]; // arr of 16, 16bit values (stores address PC should ret to)
   BYTE m_ScreenData[32][64] ;
   BYTE m_Input[16]; 
   BYTE m_DelayT;
   BYTE m_SoundT;

   BYTE m_GameMemory[0xFFF]; // just the memory
} context;

void reset(struct cpuContext *pContext) {
   pContext->m_ProgramCounter = 0x200; // the start of programs here
   //pContext->m_StackPointer = &(pContext->m_StackArr[0]); // pointer to first part of array
   pContext->m_StackPointer = 0; 
   
   pContext->m_DelayT = 0;
   pContext->m_DelayT = 0;

   // do i need to set regs?
   for (int i = 0; i < 16; i++) {
      pContext->m_Registers[i] = 0;
   }
   
   // load in the font
   FILE *in;
   in = fopen( "myfile.ch8", "rb" );
   BYTE *mem_point = pContext->m_GameMemory;
   fread(&mem_point[0x050], 0x050, 1, in);
   fclose(in);

   // load in the game
   in = fopen( "INVADERS.ch8", "rb" );
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
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
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
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
   }

   pContext->m_Registers[regNumX] = regContentsX - regContentsY;
}

// Set Vx = Vx SHR 1.
void op_8xy6(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX]; 
   if (regContentsX & 0x01 == 0x01) { // if LSB is 1, set flag
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
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
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
   }

   pContext->m_Registers[regNumX] = regContentsY - regContentsX;
}

// SHL: Set Vx = Vx SHL 1.
void op_8xyE(struct cpuContext *pContext, WORD opcode) {
   int regNumX = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContentsX = pContext->m_Registers[regNumX]; 
   if (regContentsX & 0x80 == 0x80) { // if MSB is 1, set flag. 0x80 is 1000 0000
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
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

         int byteDataBitMask = (0x01 << (7-j)); // this lets us get proper mask; e.g., b0100.0000 is "second" pixel, j = 1, so << by 6
         if (byteDataBitMask & byteData) { // if it not 0, then we toggle the onscren position
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
      pContext->m_Registers[0xf] = 1;
   } else {
      pContext->m_Registers[0xf] = 0;
   }
}

// SKP: Skip next instruction if key with the value of Vx is pressed.
void op_Ex9E(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber];
   BYTE inputContents = pContext->m_Input[regContents];
   if (inputContents == 1) { // if pressed
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// SKNP: Skip next instruction if key with the value of Vx is not pressed.
void op_ExA1(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber];
   BYTE inputContents = pContext->m_Input[regContents];
   if (inputContents == 0) { // if not pressed
      pContext->m_ProgramCounter += 2; // skip next instr
   }
}

// LD: Set Vx = delay timer value.
void op_Fx07(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_Registers[regNumber] = pContext->m_DelayT;
}

// LD: Wait for a key press, store the value of the key in Vx.
void op_Fx0A(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   int val = -1;
   SDL_Event event;
   while (val == -1 && SDL_WaitEvent(&event)) {
      if (event.type == SDL_KEYDOWN) {
         switch (event.key.keysym.sym) {
            case SDLK_1: val = 0x1; break;
            case SDLK_2: val = 0x2; break;
            case SDLK_3: val = 0x3; break;
            case SDLK_4: val = 0xC; break;
            case SDLK_q: val = 0x4; break;
            case SDLK_w: val = 0x5; break;
            case SDLK_e: val = 0x6; break;
            case SDLK_r: val = 0xD; break;
            case SDLK_a: val = 0x7; break;
            case SDLK_s: val = 0x8; break;
            case SDLK_d: val = 0x9; break;
            case SDLK_f: val = 0xE; break;
            case SDLK_z: val = 0xA; break;
            case SDLK_x: val = 0x0; break;
            case SDLK_c: val = 0xB; break;
            case SDLK_v: val = 0xF; break;

            default: printf("ERROR: IMPROPER KEYPRESS"); break;
         }
      }
   }
   pContext->m_Registers[regNumber] = val;
}

// LD: Set delay timer = Vx.
void op_Fx15(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_DelayT = pContext->m_Registers[regNumber];
}

// LD: Set sound timer = Vx.
void op_Fx18(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_SoundT = pContext->m_Registers[regNumber];
}


// ADD: Set I = I + Vx.
void op_Fx1E(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   pContext->m_AddressI = pContext->m_AddressI + pContext->m_Registers[regNumber];
}

// LD: Set I = location of sprite for digit Vx.
void op_Fx29(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber]; 
   int location = 0x50 + (regContents * 5);
   pContext->m_AddressI = location;
}

// LD: Store BCD representation of Vx in memory locations I, I+1, and I+2.
void op_Fx33(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F
   BYTE regContents = pContext->m_Registers[regNumber]; 
   
   int hundreds = (regContents / 100) % 10;
   int tens = (regContents / 10) % 10;
   int ones = regContents % 10;

   pContext->m_GameMemory[pContext->m_AddressI] = hundreds;
   pContext->m_GameMemory[pContext->m_AddressI+1] = tens;
   pContext->m_GameMemory[pContext->m_AddressI+2] = ones;
}

// LD: Store registers V0 through Vx in memory starting at location I. inclusive?
void op_Fx55(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F

   for (int i = 0; i < regNumber+1; i++) {
      pContext->m_GameMemory[pContext->m_AddressI+i] = pContext->m_Registers[i];
   }
   pContext->m_AddressI = pContext->m_AddressI+regNumber+1;
}

// LD: Read registers V0 through Vx from memory starting at location I.
void op_Fx65(struct cpuContext *pContext, WORD opcode) {
   int regNumber = (opcode & 0x0F00) >> 8; // shift right two place -> from 0x0F00 to 0x000F

   for (int i = 0; i < regNumber+1; i++) {
      pContext->m_Registers[i] = pContext->m_GameMemory[pContext->m_AddressI+i];
   }
   pContext->m_AddressI = pContext->m_AddressI+regNumber+1;
}

// END OPCODES

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
      case 0xE000:
         switch(opcode & 0x000F) {
            case 0x0001: op_ExA1(pContext, opcode); break; // sknp
            case 0x000E: op_Ex9E(pContext, opcode); break; // skp
         } break;
      case 0xF000:
         switch(opcode & 0x00F0) {
            case 0x0000: // Fx0?
               switch(opcode & 0x000F) {
                  case 0x0007: op_Fx07(pContext, opcode); break; // 
                  case 0x000A: op_Fx0A(pContext, opcode); break; // 
               } break;
            case 0x0010: // Fx1?
               switch(opcode & 0x000F) {
                  case 0x0005: op_Fx15(pContext, opcode); break; // 
                  case 0x0008: op_Fx18(pContext, opcode); break; // 
                  case 0x000E: op_Fx1E(pContext, opcode); break; // 
               } break;
            case 0x0020: op_Fx29(pContext, opcode); break; //
            case 0x0030: op_Fx33(pContext, opcode); break; //
            case 0x0050: op_Fx55(pContext, opcode); break; //
            case 0x0060: op_Fx65(pContext, opcode); break; //
         } break;
      default: printf("ERROR: UNIMPLEMENTED OPCODE"); break;
   }
}

void handleInputs(struct cpuContext *pContext) {
   memset(pContext->m_Input, 0, sizeof(pContext->m_Input)); // reset, for check again
   SDL_Event event;
   while (SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN) {
         switch (event.key.keysym.sym) {
            case SDLK_1: pContext->m_Input[0x1] = 1; break;
            case SDLK_2: pContext->m_Input[0x2] = 1; break;
            case SDLK_3: pContext->m_Input[0x3] = 1; break;
            case SDLK_4: pContext->m_Input[0xC] = 1; break;
            case SDLK_q: pContext->m_Input[0x4] = 1; break;
            case SDLK_w: pContext->m_Input[0x5] = 1; break;
            case SDLK_e: pContext->m_Input[0x6] = 1; break;
            case SDLK_r: pContext->m_Input[0xD] = 1; break;
            case SDLK_a: pContext->m_Input[0x7] = 1; break;
            case SDLK_s: pContext->m_Input[0x8] = 1; break;
            case SDLK_d: pContext->m_Input[0x9] = 1; break;
            case SDLK_f: pContext->m_Input[0xE] = 1; break;
            case SDLK_z: pContext->m_Input[0xA] = 1; break;
            case SDLK_x: pContext->m_Input[0x0] = 1; break;
            case SDLK_c: pContext->m_Input[0xB] = 1; break;
            case SDLK_v: pContext->m_Input[0xF] = 1; break;

            default: printf("ERROR: IMPROPER KEYPRESS"); break;
         }
      }
   }
   /** 
   if (pContext->m_Input[0x1] == 1) {
      printf("win" );
   } else {
      printf("fail");
   }
   */
}

void decrementTimers(struct cpuContext *pContext) {
   if (pContext->m_DelayT > 0) {
      pContext->m_DelayT -= 1;
      if (pContext->m_DelayT < 0) {
         pContext->m_DelayT = 0;
      }
   }
   if (pContext->m_SoundT > 0) {
      pContext->m_SoundT -= 1;
      if (pContext->m_SoundT < 0) {
         pContext->m_SoundT = 0;
      }
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




// 17ms per frame
int main() {
   printf("CHIP8 Booting Up...");
   int FPS = 60;
   int OPCODES_PER_FRAME = 10; 
   int MS_PER_FRAME = (1000/FPS);
   drawWindow();
   reset(&context);
   while (1) { // each of these, a frame
      decrementTimers(&context);
      int startFrameTime = SDL_GetTicks();
      int opcode_executed = 0;
      while (opcode_executed < OPCODES_PER_FRAME) {
         WORD fetchedOP = fetchOpcode(&context);
         executeOpcode(&context, fetchedOP);
         opcode_executed++;
      }
      int elapsedTime = SDL_GetTicks() - startFrameTime;
      if (elapsedTime < MS_PER_FRAME) { // if less, wait a bit longer
         SDL_Delay(MS_PER_FRAME - elapsedTime);
      }
      
      handleInputs(&context);
      drawPixels(context.m_ScreenData);
      //SDL_Delay(20);
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