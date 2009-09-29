#include "vga_sdl.h"


#ifdef __SDLSDK__
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#endif
static unsigned short int framebufferA[1024*512];

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

unsigned int shader_num;

/*Color Key fragment program*/
//char program_string[] = "\
!!ARBfp1.0  \
ATTRIB tex = fragment.texcoord[0]; \
PARAM key = {0.0,0.0,1.0,1.0}; \
PARAM c_transparent = {0.0,0.0,0.0,0.0}; \
OUTPUT c_out = result.color; \
\
TEMP c_orig , c_add, dot; \
\
TEX c_orig , tex , texture[0] , 2D; \
SUB c_add, key , c_orig;\
DP3 dot , c_add , key;\
MUL c_out , dot , c_orig;\
\
END\
";

char program_string[] = 
"!!ARBfp1.0\n"
"ATTRIB tex = fragment.texcoord[0]; \n"
"PARAM key = {1.0,0.0,1.0,1.0}; \n"
"PARAM c_transparent = {0.0,0.0,0.0,0.0}; \n"
"OUTPUT c_out = result.color; \n"

"TEMP c_orig , c_add, dot; \n"

"TEX c_orig , tex , texture[0] , 2D; \n"
"SUB c_add, key , c_orig;\n"
"DP3 dot , c_add , key;\n"
"MUL c_out , dot , c_orig;\n"

"END\n"
;

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

    GLint errPos;
    unsigned char *errString;

    glPixelStorei(GL_UNPACK_SWAP_BYTES , 1);
    glPixelStorei(GL_UNPACK_LSB_FIRST , 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT , 1);

    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);           /* Realy Nice perspective calculations */

    glClearColor (1.0f, 0.0f, 0.0f, 0.0f);        /* Light Grey Background */
    glClearDepth (1.0f);                                        /* Depth Buffer Setup */

    glDepthFunc (GL_LEQUAL);                                    /* The Type Of Depth Test To Do */
    glEnable (GL_DEPTH_TEST);                                 /* Enable Depth Testing */
   // glEnable(GL_ALPHA_TEST);

    glShadeModel (GL_SMOOTH);                                 /* Enables Smooth Color Shading */
    glDisable (GL_LINE_SMOOTH);                               /* Initially Disable Line Smoothing */

    //glEnable(GL_COLOR_MATERIAL);                            /* Enable Color Material (Allows Us To Tint Textures) */
    glDisable(GL_COLOR_MATERIAL);                            /* Enable Color Material (Allows Us To Tint Textures) */

    glEnable(GL_TEXTURE_2D);                                    /* Enable Texture Mapping */

//    glEnable(GL_LIGHT0);                                            /* Enable Light0 (Default GL Light) */

    

    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    printf("GL ERROR : %s\n", gluErrorString(glGetError()));

    glGenProgramsARB(1, &shader_num);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_num);
    printf("GL ERROR : %s\n", gluErrorString(glGetError()));

    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program_string), program_string);

if (GL_INVALID_OPERATION == glGetError())
{
    // Find the error position
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos ); 
 
    // Print implementation-dependent program
    // errors and warnings string.
    errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB); 
 
    fprintf(stderr, "error at position: %d\n%s\n", errPos, errString);
}
 //   printf("GL ERROR : %s %d\n", gluErrorString(glGetError()), strlen(program_string));

    glDisable(GL_FRAGMENT_PROGRAM_ARB);



            glViewport (0, 0,vga_hres, vga_vres);
            glMatrixMode (GL_PROJECTION);       /* Select The Projection Matrix */
            glLoadIdentity ();                          /* Reset The Projection Matrix */
            /* Set Up Ortho Mode To Fit 1/4 The Screen (Size Of A Viewport) */
            glOrtho(0, vga_hres, vga_vres, 0, -99999 , 99999);

glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);



}

void vga_init()
{
    vga_hres = 640;
    vga_vres = 480;
    int x;
    unsigned short int color;

    const SDL_VideoInfo *videoInfo;
    int videoFlags;

if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
        {
        fprintf( stderr, "Video initialization failed: %s\n",
             SDL_GetError( ) );
        SDL_Quit();
        }

    /* Fetch the video info */
    videoInfo = SDL_GetVideoInfo( );

    if ( !videoInfo )
        {
        fprintf( stderr, "Video query failed: %s\n",
             SDL_GetError( ) );
        SDL_Quit();
        }

/* the flags to pass to SDL_SetVideoMode */
    videoFlags  = SDL_OPENGL;          /* Enable OpenGL in SDL */
    videoFlags |= SDL_GL_DOUBLEBUFFER; /* Enable double buffering */
    videoFlags |= SDL_HWPALETTE;       /* Store the palette in hardware */
    videoFlags |= SDL_RESIZABLE;       /* Enable window resizing */

    /* This checks to see if surfaces can be stored in memory */
    if ( videoInfo->hw_available )
            videoFlags |= SDL_HWSURFACE;
    else
            videoFlags |= SDL_SWSURFACE;


    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

    screen = SDL_SetVideoMode( vga_hres, vga_vres, 32, videoFlags);
    SDL_WM_SetCaption("Milkymist Democompo", NULL);

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
    SDL_GL_SwapBuffers( );
}

void flush_bridge_cache()
{
    glFlush();
}
