#ifndef __DEMO_1_H
#define __DEMO_1_H
typedef struct _t_vtx3D
{
    float x , y , z;
} __attribute__((packed)) t_vtx3D;

typedef struct _t_face
{
    t_vtx3D vtx[3];
} __attribute__((packed)) t_face; 

void demo_1();
#endif //__DEMO_1_H
