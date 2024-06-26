#include <SDL2/SDL.h>
#include <stdio.h>


typedef unsigned char BYTE; // ex, 0x1F, 8 bits

SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
int drawWindow() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow("Hello World!", 100, 100, 640, 320, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL) {
        SDL_DestroyWindow(win);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);   // Set the drawing color to white
    SDL_RenderClear(ren); // "cleans" the screen with color
    SDL_RenderPresent(ren); // Update the screen

    // Wait for 5 seconds
    SDL_Delay(1000);


    return 0;
}

int drawPixel(int x, int y, int isWhite) {
    SDL_Rect rect;
    SDL_SetRenderDrawColor(ren, 255*isWhite, 255*isWhite, 255*isWhite, 255);
    rect.x = 10*x;
    rect.y = 10*y;
    rect.w = 10;
    rect.h = 10;
    SDL_RenderFillRect(ren, &rect);
    SDL_RenderPresent(ren);
    SDL_Delay(50);
    return 1;
}

int drawPixels(BYTE arr[32][64]) {
    //SDL_RenderClear(ren); // "cleans" the screen with color
    
    SDL_Rect rect;
    int rows = 32;
    int cols = 64;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            SDL_Rect rect;
            rect.x = 10*c;
            rect.y = 10*r;
            rect.w = 10;
            rect.h = 10;
           if (arr[r][c]) { // if not null?
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);   // Set the drawing color to white
                SDL_RenderFillRect(ren, &rect);
           } else { // draw opposite color?
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);   // Set the drawing color to black
                SDL_RenderFillRect(ren, &rect);
           }
        }
    }
    SDL_RenderPresent(ren);
    //SDL_Delay(5000);
    return 0;
}

int drawRect() {
    // Define a rectangle

    SDL_SetRenderDrawColor(ren, 255, 0, 255, 255);
    SDL_Rect rect;
    rect.x = 200;
    rect.y = 150;
    rect.w = 240;
    rect.h = 180;

    // Draw the rectangle
    SDL_RenderFillRect(ren, &rect);

    // Update the screen
    SDL_RenderPresent(ren);

    // Wait for 5 seconds
    SDL_Delay(5000);

    // Clean up and quit SDL
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}