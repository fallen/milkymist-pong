#ifndef __VGA_H
#define __VGA_H

//#ifdef __SDLSDK__
//#include <SDL/SDL.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
//
//#endif

extern int vga_hres;
extern int vga_vres;

extern unsigned short int *vga_frontbuffer;
extern unsigned short int *vga_backbuffer;
extern unsigned short int *vga_lastbuffer;

extern unsigned int shader_num;

void vga_init();
void vga_disable();
void vga_swap_buffers();

void flush_bridge_cache();

#endif //__VGA_H
