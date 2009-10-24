#ifndef __SDLSDK__
#include <libc.h>
#include <system.h>
#else
#include <stdlib.h>
#endif

#include "tmu.h"
#include "vga.h"
#include "angle.h"
#include "color.h"
#define CHROMAKEY  0xf81f
#define HMESHLAST  64
#define VMESHLAST  48

#define DURATION 700

static int frames;

static volatile int tmu_wait;

static void tmu_complete(struct tmu_td *td)
{
    tmu_wait = 1;
}

static unsigned short ramp[256*256];


static void make_mesh(struct tmu_vertex *src_vertices, struct tmu_vertex *dst_vertices, int frame)
{
	int x, y;
	int px, py;
	int cx, cy;

	int xscalex=15;
	int xspeedx=1;
	int xscaley=13;
	int xspeedy=9;
	int xscalez=10;
	int xspeedz=7;
	int xscalew=10;
	int xspeedw=9;
	int yscalex=3;
	int yspeedx=13;
	int yscaley=5;
	int yspeedy=10;
	int yscalez=5;
	int yspeedz=4;
	int yscalew=10;
	int yspeedw=4;

	cx = vga_hres;
	cy = vga_vres;
	for(y=0;y<=VMESHLAST;y++)
		for(x=0;x<=HMESHLAST;x++) {
		  px=50; //(1500+COS[(x*xscalez+frame*xspeedz)%360])/20;
		  py=50; //(1500+COS[(x*yscalez+frame*yspeedz)%360])/20;
		  cx=50; //(1500+COS[(x*xscalew+frame*xspeedw)%360])/20;
		  cy=50; //(1500+COS[(x*yscalew+frame*yspeedw)%360])/20;
		  src_vertices[TMU_MESH_MAXSIZE*y+x].x = 
		    (2000+
		     COS[((x*xscalex+xspeedx*px/50)*frame/20)%360]+
		     COS[((y*xscaley+xspeedy*cx/50)*frame/20)%360])>>4;
		  src_vertices[TMU_MESH_MAXSIZE*y+x].y = 
		    (2000+
		     COS[((x*yscalex+yspeedx*px/50)*frame/20)%360]+
		     COS[((y*yscaley+yspeedy*cx/50)*frame/20)%360])>>4;
		  dst_vertices[TMU_MESH_MAXSIZE*y+x].x = x*vga_hres/HMESHLAST;
		  dst_vertices[TMU_MESH_MAXSIZE*y+x].y = y*vga_vres/VMESHLAST;
		}
}


void test1()
{
	int i;
	int sn;
	/* define those as static, or the compiler optimizes the stack a bit too much */
	static struct tmu_td tmu_task;
	static struct tmu_vertex src_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
	static struct tmu_vertex dst_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
	
	frames=0;

	for(i=0;i<256*256;i++)
	  {
	    ramp[i]=MAKERGB565N(i>>8,i,0);
	    ramp[i]=(ramp[i]<<8)+(ramp[i]>>8);
	  }


	tmu_task.flags = 0;
	tmu_task.hmeshlast = HMESHLAST;
	tmu_task.vmeshlast = VMESHLAST;
	tmu_task.brightness = 62;
	tmu_task.chromakey = 0;
	tmu_task.srcmesh = &src_vertices[0][0];
	tmu_task.srchres = 256;
	tmu_task.srcvres = 256;
	tmu_task.dstmesh = &dst_vertices[0][0];
	tmu_task.dsthres = vga_hres;
	tmu_task.dstvres = vga_vres;
	tmu_task.profile = 0;
	tmu_task.callback = tmu_complete;
	tmu_task.user = NULL;

	make_mesh(&src_vertices[0][0], &dst_vertices[0][0], 100000);



	sn = 0;
	for(i=0;i<DURATION;i++) {
	  tmu_task.srcfbuf = ramp;
	  tmu_task.dstfbuf = vga_backbuffer;

	  make_mesh(&src_vertices[0][0], &dst_vertices[0][0], frames);

	  tmu_wait = 0;
	  tmu_submit_task(&tmu_task);
	  while(!tmu_wait);
	  
	  flush_bridge_cache();
		
	  vga_swap_buffers();
	  frames+=1;
	}

}
