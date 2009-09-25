#include "sprites.h"

t_sprite sprite_data[4096];
unsigned int sprite_index;

void sprites_init()
{
    sprite_index = 0;
}

void debug_sprite(t_sprite * s)
{
    printf ("SPRITE INFO\n");
    printf ("data : %X\n",(unsigned int) s->data);
    printf ("size_hres : %u\n",s->size_hres);
    printf ("size_vres : %u\n",s->size_vres);
    printf ("len : %u\n",s->len);
    printf ("colorkey : %X\n",s->colorkey);

}

void sprites_load(unsigned char * data, unsigned int data_len, unsigned int colorkey , unsigned int grid_hres, unsigned int grid_vres, unsigned numframe)
{

    unsigned int len,start;
    unsigned int nb_frames;

    len = grid_hres * grid_vres ;

    if (len > data_len)
    {
        printf("Error : grid size of image too big (%u > %u) \n", len , data_len);
        return;
    }

    if (data_len % len > 0) 
    {
        printf("Warning : Frames are not equally cut (%u %% %u = %u)\n" , data_len , len , data_len % len);
    }

    nb_frames = data_len / len; 
    
    start = numframe * len;
    if (start > data_len)
    {
        printf("Error : %d frame exceed image's total length\n", numframe);
        return;
    }   

    sprite_data[sprite_index].size_hres = grid_hres;
    sprite_data[sprite_index].size_vres = grid_vres;

    sprite_data[sprite_index].data = &((unsigned short int *) data)[start];
    sprite_data[sprite_index].len = len;
    
    sprite_data[sprite_index].colorkey = colorkey;
    sprite_index+=1;
}



void sprites_display( t_sprite * sprite)
{


}
