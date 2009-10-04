#ifndef __SPRITES_H
#define __SPRITES_H

#include <stdio.h>
#include "tmu.h"
#include "angle.h"
#define SPRITE_MAX_FRAME 100


typedef struct _t_sprite {
    char name[255];
    unsigned short int * data;
    int size_hres;
    int size_vres;
    unsigned int len;
    unsigned int colorkey;
//    unsigned int gltexid;
    
}  __attribute__((packed)) t_sprite;


typedef struct _t_matrix_2x2 {
    int e[2][2];
} __attribute__((packed)) t_matrix_2x2;

extern t_sprite sprite_data[4096];
extern unsigned int sprite_index;

void sprites_display (t_sprite * sprite);
void sprites_load(unsigned char * data, unsigned int data_len, unsigned int colorkey , unsigned int grid_hres, unsigned int grid_vres,unsigned int numframe);
void sprites_init();

void debug_sprite(t_sprite * s);

void init_sprite (struct tmu_vertex * src_vtx , struct tmu_vertex  * dst_vtx , t_sprite * sp);
void move_sprite(struct tmu_vertex * dst_vtx , int pos_x , int pos_y);
void scale_sprite(struct tmu_vertex * dst_vtx , int scale_x , int scale_y);
void rotate_sprite(struct tmu_vertex * dst_vtx , int rot);
#endif //__SPRITES_H
