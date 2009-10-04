#include "demo_1.h"
#include <system.h>

#include "tmu.h"
#include "vga.h"
#include "sprites.h"
//#include "badclouds.png.h"
//#include "badfactory.png.h"
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

void demo_1()
{
    
}
