
#ifdef __SDLSDK__ //We are debugging on a computer :)

#include "sdl_config.h"
#else // We are debugging on MilkyMist

#include "mm_config.h"

#endif
static volatile int tmu_wait;

static void tmu_complete(struct tmu_td *td)
{
    tmu_wait = 1;
}


int main() {




#ifndef __SDLSDK__
    irq_setmask(0);
    irq_enable(1);
    uart_async_init();
#endif
    banner();
#ifndef __SDLSDK__
    brd_init();
    time_init();
    mem_init();
#endif
    vga_init();
#ifndef __SDLSDK__
    snd_init();
    tmu_init();
    pfpu_init();
#endif

    static struct tmu_td tmu_task, tmu_clearscreen;
    static struct tmu_vertex src_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex dst_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    static short int black = 0xF800;


    static struct tmu_vertex src_clr_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex dst_clr_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    


    int quit = 0;
    unsigned int x , y ;

    unsigned short int * vga_position;
    //Initialization
    sprites_init();

    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,0);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,1);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,2);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,3);
    sprites_load(badclouds_raw,badclouds_raw_len,0x001F,138,100,4);
    sprites_load(badfactory_raw,badfactory_raw_len,0x001F,138,100,4);
//    debug_sprite(&sprite_data[0]);

#define CHROMAKEY  0x001f
#define HMESHLAST  1
#define VMESHLAST  1


    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            src_vertices[x][y].x = x * 138 / HMESHLAST; 
            src_vertices[x][y].y = y * 100 / VMESHLAST;
            dst_vertices[x][y].x = x * 69 / HMESHLAST + 100; 
            dst_vertices[x][y].y = y * 50 / VMESHLAST + 100;
        }
    }

    src_clr_vertices[0][0].x = 0;
    src_clr_vertices[0][0].y = 0;
    src_clr_vertices[1][0].x = 1;
    src_clr_vertices[1][0].y = 0;
    src_clr_vertices[0][1].x = 0;
    src_clr_vertices[0][1].y = 1;
    src_clr_vertices[1][1].x = 1;
    src_clr_vertices[1][1].y = 1;


    dst_clr_vertices[0][0].x = 0;
    dst_clr_vertices[0][0].y = 0;
    dst_clr_vertices[1][0].x = 640;
    dst_clr_vertices[1][0].y = 0;
    dst_clr_vertices[0][1].x = 0;
    dst_clr_vertices[0][1].y = 480;
    dst_clr_vertices[1][1].x = 640;
    dst_clr_vertices[1][1].y = 480;

    tmu_clearscreen.flags = 0;
    tmu_clearscreen.hmeshlast = 1;
    tmu_clearscreen.vmeshlast = 1;
    tmu_clearscreen.brightness = 60;
    tmu_clearscreen.chromakey = 0;
    tmu_clearscreen.srcmesh = &src_clr_vertices[0][0];
    tmu_clearscreen.srchres = 1;
    tmu_clearscreen.srcvres = 1;
    tmu_clearscreen.dstmesh = &dst_clr_vertices[0][0];
    tmu_clearscreen.dsthres = vga_hres;
    tmu_clearscreen.dstvres = vga_vres;
    tmu_clearscreen.profile = 0;
    tmu_clearscreen.callback = tmu_complete;
    tmu_clearscreen.user = NULL;

    tmu_task.flags = TMU_CTL_CHROMAKEY;
    tmu_task.hmeshlast = HMESHLAST;
    tmu_task.vmeshlast = VMESHLAST;
    tmu_task.brightness = 62;
    tmu_task.chromakey = 0x001f;
    tmu_task.srcmesh = &src_vertices[0][0];
    tmu_task.srchres = 138;
    tmu_task.srcvres = 100;
    tmu_task.dstmesh = &dst_vertices[0][0];
    tmu_task.dsthres = vga_hres;
    tmu_task.dstvres = vga_vres;
    tmu_task.profile = 0;
    tmu_task.callback = tmu_complete;
    tmu_task.user = NULL;
    x = 0;

    while(quit == 0)
    {
        tmu_task.srcfbuf = sprite_data[x/20].data;
        tmu_task.dstfbuf = vga_backbuffer;
        tmu_clearscreen.srcfbuf = &black;
        //tmu_clearscreen.srcfbuf = vga_frontbuffer;
        tmu_clearscreen.dstfbuf = vga_backbuffer;
        quit = scan_keys();
        x += 1;
    //    black = x * ;
        if (x == 100) x = 0;

        

//        for (y=0;y<sprite_data[0].size_vres;y++)
//        {
//            vga_position = &(vga_frontbuffer[x + y * vga_hres]);
//            memcpy(vga_position, &sprite_data[0].data[y * sprite_data[0].size_hres ] , sprite_data[0].size_hres * sizeof(unsigned short int) );
//        }
        tmu_wait = 0;
        tmu_submit_task(&tmu_clearscreen);
        while(!tmu_wait);

        tmu_wait = 0;
        tmu_submit_task(&tmu_task);
        while(!tmu_wait);

        flush_bridge_cache();

        vga_swap_buffers();
        demo_sleep(100);
    }

    demo_quit();
    return 0;
}




