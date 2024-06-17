#ifndef BLAH_H
#define BLAH_H

typedef unsigned char BYTE; // ex, 0x1F, 8 bits

int drawWindow(); // Declaration of function2
int drawRect();
int drawPixel(int x, int y, int isOn);
int drawPixels(BYTE arr[32][64]);
#endif