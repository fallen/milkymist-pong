#include "vga.h"

#ifdef SDLSDK
#include <SDL/SDL.h>
#endif

SDL_Surface * screen;
SDL_Surface * vga_frontbuffer;
SDL_Surface * vga_backbuffer;
SDL_Surface * vga_lastbuffer;


int vga_hres;
int vga_vres;


void vga_init()
{
    vga_hres = 640;
    vga_vres = 480;

    SDL_InitSubSystem(SDL_INIT_VIDEO);

    unsigned int gmask = 0x0000F800;
    unsigned int rmask = 0x000007E0;
    unsigned int bmask = 0x0000001F;
    unsigned int amask = 0x00000000;


    screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    vga_frontbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, vga_hres, vga_vres, 16, rmask , gmask , bmask , amask);
    vga_backbuffer = vga_lastbuffer = vga_frontbuffer;

}

void vga_disable()
{
   SDL_QuitSubSystem(SDL_INIT_VIDEO); 
}

void vga_swap_buffers()
{

    SDL_UpdateRect(vga_frontbuffer, 0,0,0,0);
    SDL_BlitSurface(vga_frontbuffer , NULL, screen , NULL);
    SDL_UpdateRect(screen, 0,0,0,0);
    SDL_Flip(vga_frontbuffer);
}
