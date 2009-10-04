#include "sprites.h"

#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#define HMESHLAST  1
#define VMESHLAST  1


t_sprite sprite_data[4096];
unsigned int sprite_index;

void sprites_init()
{
    sprite_index = 0;
    memset(sprite_data, sizeof(t_sprite) * 4096,0);
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


/*#ifdef __SDLSDK__
        glGenTextures(1,&(sprite_data[sprite_index].gltexid));
    printf("SPRITE GL ERROR : %s\n", gluErrorString(glGetError()));
        glBindTexture(GL_TEXTURE_2D,sprite_data[sprite_index].gltexid);
    printf("SPRITE GL ERROR : %s\n", gluErrorString(glGetError()));
        glTexImage2D(GL_TEXTURE_2D, 0, 4, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        //glTexImage2D(GL_TEXTURE_2D, 0, 3, grid_hres, grid_vres, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sprite_data[sprite_index].data);
    printf("SPRITE GL ERROR : %s\n", gluErrorString(glGetError()));
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grid_hres, grid_vres, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sprite_data[sprite_index].data);
    printf("SPRITE GL ERROR : %s\n", gluErrorString(glGetError()));
#endif
  */  
    sprite_data[sprite_index].colorkey = colorkey;
    sprite_index+=1;
}

void init_sprite (struct tmu_vertex * src_vtx , struct tmu_vertex  * dst_vtx , t_sprite * sp)
{
    int x, y;

    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            //Texture coord
            src_vtx[x + y * TMU_MESH_MAXSIZE].x = (x * sp->size_hres);
            src_vtx[x + y * TMU_MESH_MAXSIZE].y = (y * sp->size_vres);

            //Vertex coord
            dst_vtx[x + y * TMU_MESH_MAXSIZE].x = -sp->size_hres/2 + (x * sp->size_hres);
            dst_vtx[x + y * TMU_MESH_MAXSIZE].y = -sp->size_vres/2 + (y * sp->size_vres);
        }
    }
}

void move_sprite(struct tmu_vertex * dst_vtx , int pos_x , int pos_y)
{
    int x,y;
 
    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            dst_vtx[x + y * TMU_MESH_MAXSIZE].x += pos_x;
            dst_vtx[x + y * TMU_MESH_MAXSIZE].y += pos_y;
        }
    }
}

void scale_sprite(struct tmu_vertex * dst_vtx , int scale_x , int scale_y )
{
    int x , y;

    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            dst_vtx[x + y * TMU_MESH_MAXSIZE].x += (x * 2 - 1) * (scale_x/2);
            dst_vtx[x + y * TMU_MESH_MAXSIZE].y += (y * 2 - 1) * (scale_y/2);
        }
    }
}

void rotate_sprite(struct tmu_vertex * dst_vtx , int rot) 
{
    int x , y;

    int cx , cy;

    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            cx = dst_vtx[x + y * TMU_MESH_MAXSIZE].x;
            cy = dst_vtx[x + y * TMU_MESH_MAXSIZE].y;
            dst_vtx[x + y * TMU_MESH_MAXSIZE].x = (cx * COS[rot] + cy * SIN[rot])/1000;
            dst_vtx[x + y * TMU_MESH_MAXSIZE].y = (-cx * SIN[rot] + cy * COS[rot])/1000;
        }
    }
    
}


void sprites_display( t_sprite * sprite)
{


}
