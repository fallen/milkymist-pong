#include "vga.h"


#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#endif
static unsigned short int framebufferA[640*480];

static SDL_Surface * screen;
//static SDL_Surface * buffer;
//static SDL_Surface * buffer_ogl;
//static SDL_Surface * original;
//static SDL_Surface * converted;
//static GLuint textureID;

unsigned short int *vga_frontbuffer; /* < buffer currently displayed (or request sent to HW) */
unsigned short int *vga_backbuffer;  /* < buffer currently drawn to, never read by HW */
unsigned short int *vga_lastbuffer;  /* < buffer displayed just before (or HW finishing last scan) */

int vga_hres;
int vga_vres;


/* gets next power of two */
static int pot(int x) {
    int val = 1;
    while (val < x) {
        val *= 2;
    }
    return val;
}

static void initGL()
{
   /* Start Of User Initialization */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, vga_frontbuffer);
    printf("GL ERROR : %s\n", gluErrorString(glGetError()));

    glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);           /* Realy Nice perspective calculations */
    
    glClearColor (0.0f, 0.0f, 0.0f, 0.0f);        /* Light Grey Background */
    glClearDepth (1.0f);                                        /* Depth Buffer Setup */

    glDepthFunc (GL_LEQUAL);                                    /* The Type Of Depth Test To Do */
    glEnable (GL_DEPTH_TEST);                                 /* Enable Depth Testing */

    glShadeModel (GL_SMOOTH);                                 /* Enables Smooth Color Shading */
    glDisable (GL_LINE_SMOOTH);                               /* Initially Disable Line Smoothing */

    glEnable(GL_COLOR_MATERIAL);                            /* Enable Color Material (Allows Us To Tint Textures) */
    
    glEnable(GL_TEXTURE_2D);                                    /* Enable Texture Mapping */
 
}

void vga_init()
{
    vga_hres = 640;
    vga_vres = 480;
    int x;
    unsigned short int color;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_SWSURFACE | SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE );


    //Good code for simple SDL copy image to framebuffer
    //buffer = SDL_CreateRGBSurface(0 , vga_hres, vga_vres, 16, rmask , gmask , bmask , amask);
    //vga_backbuffer = vga_lastbuffer = vga_frontbuffer = (unsigned short int *)buffer->pixels;

    vga_backbuffer = vga_lastbuffer = vga_frontbuffer = framebufferA;
    for(x=0;x<640*480;x++) {
        color = x % 0xFFFF;
        vga_frontbuffer[x] = color;
    }

    initGL();


}


void vga_disable()
{
   SDL_QuitSubSystem(SDL_INIT_VIDEO); 
}

void vga_swap_buffers()
{

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 128, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, vga_frontbuffer);
    //printf("GL ERROR : %s\n", gluErrorString(glGetError()));

    glClear (GL_COLOR_BUFFER_BIT);

    glColor3f(1,1,1);
            glViewport (0, vga_hres, vga_vres, 0);
            glMatrixMode (GL_PROJECTION);       /* Select The Projection Matrix */
            glLoadIdentity ();                          /* Reset The Projection Matrix */
            /* Set Up Ortho Mode To Fit 1/4 The Screen (Size Of A Viewport) */
            gluOrtho2D(0, vga_hres, vga_hres, 0);

    glMatrixMode (GL_MODELVIEW);            /* Select The Modelview Matrix */
    glLoadIdentity ();                              /* Reset The Modelview Matrix */

    glClear (GL_DEPTH_BUFFER_BIT);      /* Clear Depth Buffer */

            glBegin(GL_QUADS);  /* Begin Drawing A Single Quad */
                /* We Fill The Entire 1/4 Section With A Single Textured Quad. */
                glTexCoord2f(1.0f, 0.0f); glVertex2i(vga_hres, 0              );
                glTexCoord2f(0.0f, 0.0f); glVertex2i(0,              0              );
                glTexCoord2f(0.0f, 1.0f); glVertex2i(0,              vga_vres);
                glTexCoord2f(1.0f, 1.0f); glVertex2i(vga_hres, vga_vres);
            glEnd();                        /* Done Drawing The Textured Quad */

    glFlush();

    SDL_GL_SwapBuffers( );
}
