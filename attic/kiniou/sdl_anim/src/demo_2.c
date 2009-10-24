
/*#ifdef __SDLSDK__ //We are debugging on a computer :)
#include "sdl_config.h"
#else // We are debugging on MilkyMist
#include "mm_config.h"
#endif
*/
#ifndef __SDLSDK__
#include <libc.h>
#include <system.h>
#else
#include <stdlib.h>
#endif

#include "tmu.h"
#include "vga.h"
#include "sprites.h"
#include "badclouds.png.h"
#include "badfactory.png.h"
#include "angle.h"
#define CHROMAKEY  0xf81f
#define HMESHLAST  1
#define VMESHLAST  1

static int frames;

static volatile int tmu_wait;

static void tmu_complete(struct tmu_td *td)
{
    tmu_wait = 1;
}

void create_tmu_sprite();





int demo_2() {


    static struct tmu_td tmu_badclouds, tmu_badfactory, tmu_clearscreen;
    static struct tmu_vertex badclouds_src_vtx[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex badclouds_dst_vtx[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    static struct tmu_vertex badfactory_src_vtx[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex badfactory_dst_vtx[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    static short int black = 0x0000;


    static struct tmu_vertex src_clr_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex dst_clr_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    int quit = 0 , x = 0;
    //unsigned int x , y ;

    //unsigned short int * vga_position;
    //Initialization
    sprites_init();

    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,0);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,1);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,2);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,3);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,4);
    sprites_load(badfactory_raw,badfactory_raw_len,0x001F,100,100,0);
/*    debug_sprite(&sprite_data[0]);
    debug_sprite(&sprite_data[1]);
    debug_sprite(&sprite_data[2]);
    debug_sprite(&sprite_data[3]);
    debug_sprite(&sprite_data[4]);
*/

    init_sprite(&badclouds_src_vtx[0][0] , &badclouds_dst_vtx[0][0] , &sprite_data[0]);
    init_sprite(&badfactory_src_vtx[0][0] , &badfactory_dst_vtx[0][0] , &sprite_data[5]);

/*    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            badclouds_src_vtx[x][y].x = x * 138; 
            badclouds_src_vtx[x][y].y = y * 100;
            badclouds_src_vtx[x][y].x = x * 138; 
            badclouds_src_vtx[x][y].y = y * 100;
        }
    }
*/
    src_clr_vertices[0][0].x = 0;
    src_clr_vertices[0][0].y = 0;
    src_clr_vertices[1][0].x = 640;
    src_clr_vertices[1][0].y = 0;
    src_clr_vertices[0][1].x = 0;
    src_clr_vertices[0][1].y = 480;
    src_clr_vertices[1][1].x = 640;
    src_clr_vertices[1][1].y = 480;


    dst_clr_vertices[0][0].x = 0 - 30;
    dst_clr_vertices[0][0].y = 0 - 23;
    dst_clr_vertices[1][0].x = 640 + 30 ;
    dst_clr_vertices[1][0].y = 0 - 23;
    dst_clr_vertices[0][1].x = 0 - 30;
    dst_clr_vertices[0][1].y = 480 + 23;
    dst_clr_vertices[1][1].x = 640 + 30;
    dst_clr_vertices[1][1].y = 480 + 23;

    //tmu_clearscreen.flags = 0;
    tmu_badclouds.flags = TMU_CTL_CHROMAKEY;
    tmu_clearscreen.hmeshlast = 1;
    tmu_clearscreen.vmeshlast = 1;
    tmu_clearscreen.brightness = 60;
    tmu_clearscreen.chromakey = 0;
    tmu_clearscreen.srcmesh = &src_clr_vertices[0][0];
    tmu_clearscreen.srchres = vga_hres;
    tmu_clearscreen.srcvres = vga_vres;
    tmu_clearscreen.dstmesh = &dst_clr_vertices[0][0];
    tmu_clearscreen.dsthres = vga_hres;
    tmu_clearscreen.dstvres = vga_vres;
    tmu_clearscreen.profile = 0;
    tmu_clearscreen.callback = tmu_complete;
    tmu_clearscreen.user = NULL;

    tmu_badclouds.flags = TMU_CTL_CHROMAKEY;
    tmu_badclouds.hmeshlast = HMESHLAST;
    tmu_badclouds.vmeshlast = VMESHLAST;
    tmu_badclouds.brightness = 62;
    tmu_badclouds.chromakey = CHROMAKEY;
    tmu_badclouds.srcmesh = &badclouds_src_vtx[0][0];
    tmu_badclouds.srchres = 138;
    tmu_badclouds.srcvres = 100;
    tmu_badclouds.dstmesh = &badclouds_dst_vtx[0][0];
    tmu_badclouds.dsthres = vga_hres;
    tmu_badclouds.dstvres = vga_vres;
    tmu_badclouds.profile = 0;
    tmu_badclouds.callback = tmu_complete;
    tmu_badclouds.user = NULL;



    tmu_badfactory.flags = TMU_CTL_CHROMAKEY;
    tmu_badfactory.hmeshlast = HMESHLAST;
    tmu_badfactory.vmeshlast = VMESHLAST;
    tmu_badfactory.brightness = 62;
    tmu_badfactory.chromakey = CHROMAKEY;
    tmu_badfactory.srcmesh = &badclouds_src_vtx[0][0];
    tmu_badfactory.srchres = 100;
    tmu_badfactory.srcvres = 100;
    tmu_badfactory.dstmesh = &badclouds_dst_vtx[0][0];
    tmu_badfactory.dsthres = vga_hres;
    tmu_badfactory.dstvres = vga_vres;
    tmu_badfactory.profile = 0;
    tmu_badfactory.callback = tmu_complete;
    tmu_badfactory.user = NULL;


    x = 0;
    frames = 0;

    while(quit < 10)
    {
        tmu_clearscreen.srcfbuf = vga_lastbuffer;
        tmu_clearscreen.dstfbuf = vga_backbuffer;

        tmu_wait = 0;
        tmu_submit_task(&tmu_clearscreen);
        while(!tmu_wait);
        
        //update_sprite(tmu_badclouds.dstmesh ,&sprite_data[x/20], 320 , 240 , frames);
        if (x/40 == 5) { 
        init_sprite(tmu_badfactory.srcmesh,tmu_badclouds.dstmesh ,&sprite_data[x/40]);
        scale_sprite(tmu_badfactory.dstmesh , (100 + COS[frames])/10 , (100 + COS[frames])/10 ); 
        rotate_sprite(tmu_badfactory.dstmesh , frames); 
        move_sprite(tmu_badfactory.dstmesh , 320 , 240);
        tmu_badfactory.srcfbuf = sprite_data[x/40].data;
        tmu_badfactory.dstfbuf = vga_backbuffer;
        } else {
        init_sprite(tmu_badclouds.srcmesh,tmu_badclouds.dstmesh ,&sprite_data[x/40]);
        scale_sprite(tmu_badclouds.dstmesh , (100 + COS[frames])/10 , (100 + COS[frames])/10 ); 
        rotate_sprite(tmu_badclouds.dstmesh , frames); 
        move_sprite(tmu_badclouds.dstmesh , 320 , 240);
        tmu_badclouds.srcfbuf = sprite_data[x/40].data;
        tmu_badclouds.dstfbuf = vga_backbuffer;
        }
/* 
        tmu_clearscreen.srcfbuf = &black;
        tmu_clearscreen.dstfbuf = vga_backbuffer;

        tmu_wait = 0;
        tmu_submit_task(&tmu_clearscreen);
        while(!tmu_wait);
*/

        tmu_wait = 0;
        tmu_submit_task(&tmu_badclouds);
        while(!tmu_wait);
        flush_bridge_cache();

        vga_swap_buffers();
//        demo_sleep(10);
        x += 1;
        frames+=5;
        if (frames > 360) frames = frames - 360;
        if (x == 240) {x = 0;quit++;}
    }

    return 0;
}




