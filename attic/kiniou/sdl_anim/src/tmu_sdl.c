#include "tmu_sdl.h"

#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "vga_sdl.h"
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

void tmu_isr(struct tmu_td *td)
{
    
    unsigned int hmesh_idx,vmesh_idx;
    int i;
    struct tmu_vertex * srcmesh;
    struct tmu_vertex * dstmesh;
    

    srcmesh = td->srcmesh;
    dstmesh = td->dstmesh;

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, td->srchres, td->srcvres, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, td->srcfbuf);

    glClear (GL_COLOR_BUFFER_BIT);                  /* Clear Screen */

    glMatrixMode (GL_MODELVIEW);            /* Select The Modelview Matrix */
    glLoadIdentity ();                              /* Reset The Modelview Matrix */
//    glClear (GL_DEPTH_BUFFER_BIT);      /* Clear Depth Buffer */

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_num);

//    glDisable(GL_TEXTURE_2D);
//    glColor3f(1.0,1.0,1.0);

    glBegin(GL_TRIANGLE_STRIP);
    printf("BEGIN\n");

//    for (hmesh_idx=0; hmesh_idx<=td->hmeshlast;hmesh_idx++)
//    {
//        for (vmesh_idx=0; vmesh_idx<=td->vmeshlast;vmesh_idx++)
//        {
//            printf("POUET %d %d %d %d\n", hmesh_idx , vmesh_idx, td->hmeshlast, td->vmeshlast);
//            printf("TEX %f %f \n" , (float) (srcmesh[hmesh_idx][vmesh_idx].x)/1024.0 , (float) (srcmesh[hmesh_idx][vmesh_idx].y)/512.0);
//            glTexCoord2f((float) (srcmesh[hmesh_idx][vmesh_idx].x)/1024.0 , (float) (srcmesh[hmesh_idx][vmesh_idx].y)/512.0 );
//            printf("POUET %d %d : %d %d\n" , hmesh_idx, vmesh_idx ,dstmesh[hmesh_idx][vmesh_idx].x , dstmesh[hmesh_idx][vmesh_idx].y);
        for (i = 0 ; i < 4 ; i++) {
            printf("POUET %x %d %d\n", dstmesh,  dstmesh[i].x , dstmesh[i].y);
            dstmesh++;
            }
//            glVertex2i( (int)dstmesh[hmesh_idx][vmesh_idx].x , (int)dstmesh[hmesh_idx][vmesh_idx].y);
//        }
//    }
/*            glBegin(GL_TRIANGLE_STRIP);
                glTexCoord2f(0,0);glVertex2i(0, 0);
                glTexCoord2f(0,100.0/512.0);glVertex2i(0, 100);
                glTexCoord2f(138.0/1024.0,0);glVertex2i(138, 0);
                glTexCoord2f(138.0/1024.0,100.0/512.0);glVertex2i(138, 100);
            glEnd();                        
*/


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
        tmu_isr(td);
		tmu_start(td);
	}

	return 1;

}

