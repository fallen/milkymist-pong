#include "vga.h"

static SDL_Surface * screen;
static SDL_Surface * buffer;

unsigned short int *vga_frontbuffer; /* < buffer currently displayed (or request sent to HW) */
unsigned short int *vga_backbuffer;  /* < buffer currently drawn to, never read by HW */
unsigned short int *vga_lastbuffer;  /* < buffer displayed just before (or HW finishing last scan) */

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

//    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
//    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
//    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
//    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
//    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );




    //screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL);
    screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_HWSURFACE | SDL_DOUBLEBUF );
    buffer = SDL_CreateRGBSurface(SDL_HWSURFACE, vga_hres, vga_vres, 16, rmask , gmask , bmask , amask);
    vga_backbuffer = vga_lastbuffer = vga_frontbuffer = (unsigned short int *)buffer->pixels;

}

void vga_disable()
{
   SDL_QuitSubSystem(SDL_INIT_VIDEO); 
}

void vga_swap_buffers()
{

    SDL_UpdateRect(buffer, 0,0,0,0);
    SDL_BlitSurface(buffer , NULL, screen , NULL);
    SDL_UpdateRect(screen, 0,0,0,0);
    SDL_Flip(screen);
    //SDL_GL_SwapBuffers( );
}
