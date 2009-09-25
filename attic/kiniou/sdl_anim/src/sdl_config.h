#ifndef __SDL_CONFIG
#define __SDL_CONFIG

#include <SDL/SDL.h>
#include <stdio.h>

#include "vga_sdl.h"
#include "tmu_sdl.h"

#include "demo_1.h"
#include "sprites.h"
#include "transition.h"

#include "badclouds.png.h"
#include "badfactory.png.h"

static SDL_Event event;

static void banner()
{
    printf("\n\n\e[1m     |\\  /|'||      |\\  /|'   |\n"
              "     | \\/ ||||_/\\  /| \\/ ||(~~|~\n"
              "     |    |||| \\ \\/ |    ||_) |\n"
              "                _/\n"
              "\e[0m           MAIN#4 DemoCompo\n\n\n");
}


int scan_keys() {
    
        while ( SDL_PollEvent(&event) )
        {
            switch( event.type ){
                case SDL_KEYDOWN:
                    printf( "Key press detected\n" );
                    switch( event.key.keysym.sym ){
                        case SDLK_ESCAPE:
                            return 1;
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
                

        }
    return 0;
}

void demo_sleep(int ms) {
    SDL_Delay(ms);
}

void demo_quit() {
    SDL_Quit();
}

#endif //__SDL_CONFIG
