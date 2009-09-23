
#ifdef __SDLSDK__ //We are debugging on a computer :)

#include "sdl_config.h"
#else // We are debugging on MilkyMist

#include "mm_config.h"

#endif



int main() {

#ifndef __SDLSDK__
    irq_setmask(0);
    irq_enable(1);
    uart_async_init();
    banner();
    brd_init();
    time_init();
    mem_init();
#endif
    vga_init();
#ifndef __SDLSDK__
    snd_init();
    tmu_init();
    pfpu_init();
#endif

    //Initialization
    sprites_init();

    sprites_load(badclouds_raw,badclouds_raw_len,0xFFFF,138,100,4);
    debug_sprite(&sprite_data[0]);


    int quit = 0;
    unsigned int x , y ;

    unsigned short int * vga_position;

    while(quit == 0)
    {
        quit = scan_keys();
        x = 50;

        for (y=0;y<sprite_data[0].size_vres;y++)
        {
            vga_position = &(vga_frontbuffer[x + y * vga_hres]);
            memcpy(vga_position, &sprite_data[0].data[y * sprite_data[0].size_hres ] , sprite_data[0].size_hres * sizeof(unsigned short int) );
        }
        vga_swap_buffers();
        demo_sleep(100);
    }

    demo_quit();
    return 0;
}




