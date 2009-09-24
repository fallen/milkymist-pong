
#ifdef __SDLSDK__ //We are debugging on a computer :)

#include "sdl_config.h"
#else // We are debugging on MilkyMist

#include "mm_config.h"

#endif
static volatile int tmu_wait;

static int tmu_complete(struct tmu_td *td)
{
    tmu_wait = 1;
}


int main() {




#ifndef __SDLSDK__
    irq_setmask(0);
    irq_enable(1);
    uart_async_init();
    banner();
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

    static struct tmu_td tmu_task;
    static struct tmu_vertex src_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];
    static struct tmu_vertex dst_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE];

    int quit = 0;
    unsigned int x , y ;

    unsigned short int * vga_position;
    //Initialization
    sprites_init();

    sprites_load(badclouds_raw,badclouds_raw_len,0xFFFF,138,100,4);
    debug_sprite(&sprite_data[0]);

#define CHROMAKEY  0x001f
#define HMESHLAST  1
#define VMESHLAST  1


    for ( x = 0; x <= HMESHLAST; x++) {
        for ( y = 0; y <= VMESHLAST; y++) {
            src_vertices[x][y].x = x * 138 / HMESHLAST; 
            src_vertices[x][y].y = y * 100 / VMESHLAST;
            dst_vertices[x][y].x = x * 138 / HMESHLAST + 100; 
            dst_vertices[x][y].y = y * 100 / VMESHLAST + 100;
        }
    }

    tmu_task.flags = 0;
    tmu_task.hmeshlast = HMESHLAST;
    tmu_task.vmeshlast = VMESHLAST;
    tmu_task.brightness = 62;
    tmu_task.chromakey = 0;
    tmu_task.srcmesh = &src_vertices[0][0];
    tmu_task.srchres = 138;
    tmu_task.srcvres = 100;
    tmu_task.dstmesh = &dst_vertices[0][0];
    tmu_task.dsthres = vga_hres;
    tmu_task.dstvres = vga_vres;
    tmu_task.profile = 0;
    tmu_task.callback = tmu_complete;
    tmu_task.user = NULL;




    while(quit == 0)
    {
        tmu_task.srcfbuf = sprite_data[0].data;
        tmu_task.dstfbuf = vga_backbuffer;
        quit = scan_keys();
        x = 50;

//        for (y=0;y<sprite_data[0].size_vres;y++)
//        {
//            vga_position = &(vga_frontbuffer[x + y * vga_hres]);
//            memcpy(vga_position, &sprite_data[0].data[y * sprite_data[0].size_hres ] , sprite_data[0].size_hres * sizeof(unsigned short int) );
//        }

        tmu_wait = 0;
        tmu_submit_task(&tmu_task);
        while(!tmu_wait);

        //flush_bridge_cache();

        vga_swap_buffers();
        //demo_sleep(100);
    }

    demo_quit();
    return 0;
}




