#include <stdlib.h>
#include <stdio.h>
#include <system.h>

#include <hal/tmu.h>
#include <hal/vga.h>

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
	int px, py, pz, pw;
	int cx, cy;

	static int xposx=0;
	int xscalex=153*2;
	int xspeedx=215;
	static int xposy=0;
	int xscaley=132*2;
	int xspeedy=195;
	static int xposz=0;
	int xscalez=101;
	int xspeedz=175;
	static int xposw=0;
	int xscalew=103;
	int xspeedw=124;

	static int yposx=0;
	int yscalex=233*2;
	int yspeedx=134;
	static int yposy=0;
	int yscaley=215*2;
	int yspeedy=176;
	static int yposz=0;
	int yscalez=195;
	int yspeedz=146;
	static int yposw=0;
	int yscalew=190;
	int yspeedw=184;

	xposx+=xspeedx;
	xposy+=xspeedy;
	xposz+=xspeedz;
	xposw+=xspeedw;

	yposx+=yspeedx;
	yposy+=yspeedy;
	yposz+=yspeedz;
	yposw+=yspeedw;

	cx = vga_hres;
	cy = vga_vres;
	for(y=0;y<=VMESHLAST;y++)
		for(x=0;x<=HMESHLAST;x++) {
			px=2000+COS[((x*xscalez+xposz)>>5)%360];
			py=2000+COS[((x*yscalez+yposz)>>5)%360];
			pw=2000+COS[((x*xscalew+xposw)>>5)%360];
			pz=2000+COS[((x*yscalew+yposw)>>5)%360];
			src_vertices[TMU_MESH_MAXSIZE*y+x].x =
				(4000
				+COS[(((x*xscalex*px>>10)+xposx)>>5)%360]
				+COS[(((y*xscaley)+xposy)>>5)%360]
				+COS[(((y*yscaley*py>>10)+yposy)>>5)%360]
				+COS[(((x*yscalex)+yposx)>>5)%360]
				)>>5;
			src_vertices[TMU_MESH_MAXSIZE*y+x].y =
				(4000
				+COS[(((x*xscalew*pw>>10)+xposw)>>5)%360]
				+COS[(((y*yscalez*pz>>10)+yposz)>>5)%360]
				)>>5;
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
	
	frames = 0;

	for(i=0;i<256*256;i++)
	    ramp[i]=MAKERGB565N(((~i&0xff)*(~i>>8))>>8,((i&0xff)*(~i>>8))>>8,~i);

	flush_bridge_cache();

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
		vga_swap_buffers();
		frames++;
	}
}
