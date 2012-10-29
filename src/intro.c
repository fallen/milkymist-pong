/*
 * Milkymist Democompo
 * Copyright (C) 2007, 2008, 2009 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <system.h>

#include <hal/vga.h>
#include <hal/tmu.h>

#include "text.h"
#include "color.h"
#include "intro.h"

#define NSTRINGS 42
static const char csv_strings[NSTRINGS][24] = {
	"1251311706,1566,5908",
	"1251311713,1561,5916",
	"1251311720,1535,5910",
	"1251311727,1513,5909",
	"1251311734,1502,5910",
	"1251311741,1500,5918",
	"1251311748,1507,5921",
	"1251312574,1585,6005",
	"1251312582,1603,6018",
	"1251312590,1599,6024",
	"1251312598,1586,6028",
	"1251312605,1571,6030",
	"1251312612,1561,6026",
	"1251312620,1552,6024",
	"1251312628,1539,6021",
	"1251312637,1539,6024",
	"1251312644,1548,6026",
	"1251312652,1555,6032",
	"1251312659,1556,6031",
	"1251402451,1262,5299",
	"1251402459,1261,5297",
	"1251402466,1260,5295",
	"1251402473,1259,5293",
	"1251402480,1258,5290",
	"1251402487,1256,5283",
	"1251402495,1256,5284",
	"1251402502,1257,5286",
	"1251402509,1259,5284",
	"1251402516,1261,5285",
	"1251402523,1261,5279",
	"1251402530,1260,5275",
	"1251402537,1261,5274",
	"1251402546,1263,5276",
	"1251402554,1262,5274",
	"1251402561,1261,5273",
	"1251402568,1259,5270",
	"1251402576,1259,5270",
	"1251402583,1261,5272",
	"1251402590,1262,5272",
	"1251402598,1261,5268",
	"1251402605,1260,5267",
	"1251402612,1258,5263"
};

#define HMESHLAST 32
#define VMESHLAST 24

static volatile int tmu_wait;

static void tmu_complete(struct tmu_td *td)
{
	tmu_wait = 1;
}

static void make_mesh(struct tmu_vertex *src_vertices, struct tmu_vertex *dst_vertices, int scale)
{
	int x, y;
	int px, py;
	int cx, cy;

	cx = vga_hres;
	cy = vga_vres;
	for(y=0;y<=VMESHLAST;y++)
		for(x=0;x<=HMESHLAST;x++) {
			px = 100000*(x*vga_hres-(cx*HMESHLAST))/(HMESHLAST*scale) + cx;
			py = 100000*(y*vga_vres-(cy*VMESHLAST))/(VMESHLAST*scale) + cy;
			src_vertices[TMU_MESH_MAXSIZE*y+x].x = px;
			src_vertices[TMU_MESH_MAXSIZE*y+x].y = py - 3;
			dst_vertices[TMU_MESH_MAXSIZE*y+x].x = x*vga_hres/HMESHLAST;
			dst_vertices[TMU_MESH_MAXSIZE*y+x].y = y*vga_vres/VMESHLAST;
		}
}

#define DURATION 700
#define POST 100

void intro_csv(void)
{
	int i;
	int sn;
	int r, g, b;
	/* define those as static, or the compiler optimizes the stack a bit too much */
	static struct tmu_td tmu_task;
	static struct tmu_vertex src_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
	static struct tmu_vertex dst_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

	tmu_task.flags = 0;
	tmu_task.hmeshlast = HMESHLAST;
	tmu_task.vmeshlast = VMESHLAST;
	tmu_task.brightness = 62;
	tmu_task.chromakey = 0;
	tmu_task.srcmesh = &src_vertices[0][0];
	tmu_task.srchres = vga_hres;
	tmu_task.srcvres = vga_vres;
	tmu_task.dstmesh = &dst_vertices[0][0];
	tmu_task.dsthres = vga_hres;
	tmu_task.dstvres = vga_vres;
	tmu_task.profile = 0;
	tmu_task.callback = tmu_complete;
	tmu_task.user = NULL;

	make_mesh(&src_vertices[0][0], &dst_vertices[0][0], 100000);

	sn = 0;
	for(i=0;i<DURATION;i++) {
		tmu_task.srcfbuf = vga_frontbuffer;
		tmu_task.dstfbuf = vga_backbuffer;

		if(i > (DURATION/3))
			make_mesh(&src_vertices[0][0], &dst_vertices[0][0], 100000-(4000*(i-DURATION/3)/DURATION));

		if(i > (DURATION/2)) {
			r = 2*(DURATION-(i-DURATION/2))/DURATION;
			g = 63*(DURATION-(i-DURATION/2))/DURATION;
			b = 0;
		} else {
			r = 2;
			g = 63;
			b = 0;
		}

		tmu_wait = 0;
		tmu_submit_task(&tmu_task);
		while(!tmu_wait);

		if((rand() % 4) == 0) {
			draw_text((rand() % vga_hres) + 1 , (rand() % vga_vres) + 2, MAKERGB565(r, g, b), csv_strings[sn]);
			sn++;
			if(sn == NSTRINGS) sn = 0;
			flush_bridge_cache();
		}
		
		vga_swap_buffers();
	}

	for(i=0;i<POST;i++) {
		tmu_task.srcfbuf = vga_frontbuffer;
		tmu_task.dstfbuf = vga_backbuffer;
		tmu_wait = 0;
		tmu_submit_task(&tmu_task);
		while(!tmu_wait);
		vga_swap_buffers();
	}
}
