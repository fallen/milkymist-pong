#include "vga.h"


#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#endif
//static unsigned short int framebufferA[640*480] __attribute__((aligned(32)));
static unsigned short int framebufferA[640*480];

static SDL_Surface * screen;
static SDL_Surface * buffer;
//static SDL_Surface * buffer_ogl;
static GLuint textureID;

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


void vga_init()
{
    vga_hres = 640;
    vga_vres = 480;

    SDL_InitSubSystem(SDL_INIT_VIDEO);

    unsigned int gmask = 0x0000F800;
    unsigned int rmask = 0x000007E0;
    unsigned int bmask = 0x0000001F;
    unsigned int amask = 0x00000000;

    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_HWSURFACE | SDL_OPENGL);

    //screen = SDL_SetVideoMode( vga_hres, vga_vres, 16, SDL_HWSURFACE | SDL_DOUBLEBUF );
    //buffer = SDL_CreateRGBSurface(SDL_HWSURFACE , vga_hres, vga_vres, 16, rmask , gmask , bmask , amask);

    //Good ones for SDL
    //buffer = SDL_CreateRGBSurface(0 , vga_hres, vga_vres, 16, rmask , gmask , bmask , amask);
    //vga_backbuffer = vga_lastbuffer = vga_frontbuffer = (unsigned short int *)buffer->pixels;

    //buffer = SDL_LoadBMP("badclouds.bmp");
//    buffer = SDL_CreateRGBSurface(SDL_HWSURFACE,vga_hres,vga_vres, 16 , 0x000000FF, 0x0000FF00 , 0x00FF0000 , 0);
    vga_backbuffer = vga_lastbuffer = vga_frontbuffer = framebufferA;

    /* Our shading model--Gouraud (smooth). */
//    glShadeModel( GL_SMOOTH );

    /* Culling. */
//    glCullFace( GL_NONE );
//    glFrontFace( GL_CCW );
    glDisable( GL_CULL_FACE );
    glEnable(GL_TEXTURE_2D);
    /* Set the clear color. */
    glClearColor( 0, 0, 0, 0 );

    /* Setup our viewport. */
    glViewport( 0, 0, vga_hres, vga_vres );

    /*
     * Change to the projection matrix and set
     * our viewing volume.
     */
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    /*
     * EXERCISE:
     * Replace this with a call to glFrustum.
     */
    //gluPerspective( 60.0, ratio, 1.0, 1024.0 );
    
    glOrtho(0.0f,vga_hres,vga_vres,0.0f,-1.0f,1.0f);
    int x;
    unsigned short color;
    for(x=0;x<640*480;x++) {
        color = 0x0F0F;
//        printf("%d %X\n",x,color);
        vga_frontbuffer[x] = color;
    }
    //SDL_UpdateRect(buffer, 0,0,0,0);


    glMatrixMode( GL_TEXTURE );
    glLoadIdentity( );
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glGenTextures(1, &textureID);

    printf("TOTO\n");
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, pot(640), pot(480), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, vga_frontbuffer);
    printf("GL ERROR : %s\n", gluErrorString(glGetError()));
    printf("TOTO\n");


}

void vga_disable()
{
   SDL_QuitSubSystem(SDL_INIT_VIDEO); 
}

void vga_swap_buffers()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // SDL_UpdateRect(buffer, 0,0,0,0);
//    SDL_UnlockSurface(buffer);
//    SDL_BlitSurface(buffer,0,buffer_ogl,0);
//    glMatrixMode( GL_TEXTURE );
 //   SDL_BlitSurface(buffer , NULL, screen , NULL);

    /* Clear the color and depth buffers. */

    /* We don't want to modify the projection matrix. */
      glMatrixMode( GL_MODELVIEW );
      glLoadIdentity( );
//    glPushMatrix();

//    glRasterPos2i(0,0);
//    SDL_LockSurface(buffer);
//    glDrawPixels(640, 480,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,(void *)vga_frontbuffer);
//    SDL_UnlockSurface(buffer);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
      glBindTexture(GL_TEXTURE_2D, textureID);
//    printf("%d %d \n" , pot(buffer->w), pot(buffer->h));
//      glTexImage2D(GL_TEXTURE_2D, 0, 3, pot(buffer->w), pot(buffer->h), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5 , buffer->pixels);
//    printf("%s\n", gluErrorString(glGetError()));

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0,0); glVertex2f(0,0);
    glTexCoord2f(0,1.0); glVertex2f(0,vga_vres/2);
    glTexCoord2f(1.0,0); glVertex2f(vga_hres/2,0);
    glTexCoord2f(1.0,1.0); glVertex2f(vga_hres/2,vga_vres/2);
    glEnd();

//    glPopMatrix();

//    SDL_UpdateRect(screen, 0,0,0,0);
//    SDL_Flip(screen);
    SDL_GL_SwapBuffers( );
//    SDL_LockSurface(buffer);
}
