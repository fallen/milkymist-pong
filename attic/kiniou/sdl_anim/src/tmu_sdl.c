#include "tmu.h"

#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#endif

#define TMU_TASKQ_SIZE 4 /* < must be a power of 2 */
#define TMU_TASKQ_MASK (TMU_TASKQ_SIZE-1)

static struct tmu_td *queue[TMU_TASKQ_SIZE];
static unsigned int produce;
static unsigned int consume;
static unsigned int level;
static int cts;

void tmu_init()
{
    produce = 0;
    consume = 0;
    level = 0;
    cts = 1;


}

static void tmu_start(struct tmu_td *td)
{

}

void tmu_isr()
{

}

int tmu_submit_task(struct tmu_td *td)
{

}

