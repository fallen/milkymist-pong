#include "tmu_sdl.h"

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
	if(queue[consume]->callback)
		queue[consume]->callback(queue[consume]);

	consume = (consume + 1) & TMU_TASKQ_MASK;
	level--;
	if(level > 0)
		tmu_start(queue[consume]); /* IRQ automatically acked */
	else {
		cts = 1;
	//	CSR_TMU_CTL = 0; /* Ack IRQ */
	}
}

void tmu_isr()
{
    

    glBegin(GL_TRIANGLE_STRIP);




    glEnd();
}

int tmu_submit_task(struct tmu_td *td)
{
	if(level >= TMU_TASKQ_SIZE) {
		printf("TMU: taskq overflow\n");
		return 0;
	}

	queue[produce] = td;
	produce = (produce + 1) & TMU_TASKQ_MASK;
	level++;

	if(cts) {
		cts = 0;
        tmu_isr();
		tmu_start(td);
	}

	return 1;

}

