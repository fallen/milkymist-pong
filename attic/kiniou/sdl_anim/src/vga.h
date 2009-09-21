#ifndef __VGA_H
#define __VGA_H

#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>


#endif

extern int vga_hres;
extern int vga_vres;

extern SDL_Surface * vga_frontbuffer;
extern SDL_Surface * vga_backbuffer;
extern SDL_Surface * vga_lastbuffer;
extern SDL_Surface * screen;

void vga_init();
void vga_disable();
void vga_swap_buffers();

#endif //__VGA_H
