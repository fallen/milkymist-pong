
#ifdef SDLSDK
#include <stdio.h> //<-replace it with console.h from Milkymist
#include <SDL/SDL.h>
#endif


#include "demo_1.h"
#include "sprites.h"
#include "transition.h"
#include "vga.h"

#include "../data/badclouds.png.h"

int main() {

    //Initialization
    vga_init();
    sprites_init();

    sprites_load(data_badclouds_raw,data_badclouds_raw_len,0xFFFF,138,100,4);
    debug_sprite(&sprite_data[0]);


    int quit = 0;
    unsigned int x , y ;

    unsigned short int * vga_position;

    SDL_Event event;

    while(quit == 0)
    {

        while ( SDL_PollEvent(&event) )
        {
            switch( event.type ){
                case SDL_KEYDOWN:
                    printf( "Key press detected\n" );
                    switch( event.key.keysym.sym ){
                        case SDLK_ESCAPE:
                            quit=1;
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
                

        }
    x = 50;

    for (y=0;y<sprite_data[0].size_vres;y++)
    {
        vga_position = &((unsigned short int *)vga_frontbuffer->pixels)[x + y * vga_hres];
        memcpy(vga_position, &sprite_data[0].data[y * sprite_data[0].size_hres ] , sprite_data[0].size_hres * sizeof(unsigned short int) );
    }
    vga_swap_buffers();
    SDL_Delay(100);
    }

    SDL_Quit();
    return 0;
}




