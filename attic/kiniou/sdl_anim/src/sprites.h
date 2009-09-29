#ifndef __SPRITES_H
#define __SPRITES_H

#include <stdio.h>

#define SPRITE_MAX_FRAME 100


typedef struct _t_sprite {
    char name[255];
    unsigned short int * data;
    int size_hres;
    int size_vres;
    unsigned int len;
    unsigned int colorkey;
    unsigned int gltexid;

} t_sprite;


extern t_sprite sprite_data[4096];
extern unsigned int sprite_index;

void sprites_display (t_sprite * sprite);
void sprites_load(unsigned char * data, unsigned int data_len, unsigned int colorkey , unsigned int grid_hres, unsigned int grid_vres,unsigned int numframe);
void sprites_init();

void debug_sprite(t_sprite * s);

#endif //__SPRITES_H
