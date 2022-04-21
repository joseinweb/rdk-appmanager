/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 * Copyright 2005-2018 John Robinson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2012-2013 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/time.h>


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "wayland-util.h"
#include "simpleshell-client-protocol.h"
#include "wayland-egl.h"
#include "../helpers/serviceconnector.h"

#define UNUSED(x) ((void)x)

static void registryHandleGlobal(void *data, 
                                 struct wl_registry *registry, uint32_t id,
		                           const char *interface, uint32_t version);
static void registryHandleGlobalRemove(void *data, 
                                       struct wl_registry *registry,
			                              uint32_t name);

static const struct wl_registry_listener registryListener = 
{
	registryHandleGlobal,
	registryHandleGlobalRemove
};

static void shellSurfaceId(void *data,
                           struct wl_simple_shell *wl_simple_shell,
                           struct wl_surface *surface,
                           uint32_t surfaceId);
static void shellSurfaceCreated(void *data,
                                struct wl_simple_shell *wl_simple_shell,
                                uint32_t surfaceId,
                                const char *name);
static void shellSurfaceDestroyed(void *data,
                                  struct wl_simple_shell *wl_simple_shell,
                                  uint32_t surfaceId,
                                  const char *name);
static void shellSurfaceStatus(void *data,
                               struct wl_simple_shell *wl_simple_shell,
                               uint32_t surfaceId,
                               const char *name,
                               uint32_t visible,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               wl_fixed_t opacity,
                               wl_fixed_t zorder);
static void shellGetSurfacesDone(void *data,
                                 struct wl_simple_shell *wl_simple_shell);

static const struct wl_simple_shell_listener shellListener = 
{
   shellSurfaceId,
   shellSurfaceCreated,
   shellSurfaceDestroyed,
   shellSurfaceStatus,
   shellGetSurfacesDone
};

typedef enum _InputState
{
   InputState_main,
   InputState_attribute,
} InputState;

typedef enum _Attribute
{
   Attribute_position,
   Attribute_size,
   Attribute_visibility,
   Attribute_opacity,
   Attribute_zorder
} Attribute;

typedef struct _AppCtx
{
   struct wl_display *display;
   struct wl_registry *registry;
   struct wl_shm *shm;
   struct wl_compositor *compositor;
   struct wl_simple_shell *shell;
   struct wl_seat *seat;
   struct wl_surface *surface;
   struct wl_output *output;
   struct wl_egl_window *native;
   struct wl_callback *frameCallback;


   EGLDisplay eglDisplay;
   EGLConfig eglConfig;
   EGLSurface eglSurfaceWindow;
   EGLContext eglContext;   
   EGLImageKHR eglImage;
   EGLNativePixmapType eglPixmap;

   bool getShell;
   InputState inputState;
   Attribute attribute;
   
   int planeX;
   int planeY;
   int planeWidth;
   int planeHeight;

   uint32_t surfaceIdOther;
   uint32_t surfaceIdCurrent;
   float surfaceOpacity;
   float surfaceZOrder;
   bool surfaceVisible;   
   int surfaceX;
   int surfaceY;
   int surfaceWidth;
   int surfaceHeight;
   
   int surfaceDX;
   int surfaceDY;
   int surfaceDWidth;
   int surfaceDHeight;

	struct 
	{
		GLuint rotation_uniform;
		GLuint pos;
		GLuint col;
	} gl;
	long long startTime;
	long long currTime;
	bool noAnimation;
	bool needRedraw;
	bool verboseLog;
} AppCtx;

static bool setupEGL( AppCtx *ctx );
static void termEGL( AppCtx *ctx );
static bool createSurface( AppCtx *ctx );
static void resizeSurface( AppCtx *ctx, int dx, int dy, int width, int height );
static void destroySurface( AppCtx *ctx );
static bool setupGL( AppCtx *ctx );

int g_running= 0;
int g_log= 0;

static void signalHandler(int signum)
{
   printf("signalHandler: signum %d\n", signum);
	g_running = 0;
}

static long long currentTimeMillis()
{
   long long timeMillis;
   struct timeval tv;   

   gettimeofday(&tv, NULL);
   timeMillis = tv.tv_sec * 1000 + tv.tv_usec / 1000;
   
   return timeMillis;
}

static void shmFormat(void *data, struct wl_shm *wl_shm, uint32_t format)
{
   //AppCtx *ctx = (AppCtx*)data;

   printf("shm format: %X\n", format);
}

struct wl_shm_listener shmListener = {
	shmFormat
};

static void seatCapabilities( void *data, struct wl_seat *seat, uint32_t capabilities )
{
//	AppCtx *ctx = (AppCtx*)data;

   printf("seat %p caps: %X\n", seat, capabilities );
   
   if ( capabilities & WL_SEAT_CAPABILITY_KEYBOARD )
   {
      printf("  seat has keyboard\n");
   }
   if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
   {
      printf("  seat has pointer\n");
   }
   if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
   {
      printf("  seat has touch\n");
   }   
}

static void seatName( void *data, struct wl_seat *seat, const char *name )
{
	//AppCtx *ctx = (AppCtx*)data;
   printf("seat %p name: %s\n", seat, name);
}

static const struct wl_seat_listener seatListener = {
   seatCapabilities,
   seatName 
};

static void outputGeometry( void *data, struct wl_output *output, int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height, int32_t subpixel,
                            const char *make, const char *model, int32_t transform )
{
   UNUSED(data);
   UNUSED(output);
   UNUSED(x);
   UNUSED(y);
   UNUSED(physical_width);
   UNUSED(physical_height);
   UNUSED(subpixel);
   UNUSED(make);
   UNUSED(model);
   UNUSED(transform);
}

static void outputMode( void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh )
{
	AppCtx *ctx = (AppCtx*)data;

   if ( flags & WL_OUTPUT_MODE_CURRENT )
   {
      if ( (width !=  ctx->planeWidth) || (height != ctx->planeHeight) )
      {
         ctx->planeWidth= width;
         ctx->planeHeight= height;
         if ( ctx->verboseLog )
         {
            printf("outputMode: resize egl window to (%d,%d)\n", ctx->planeWidth, ctx->planeHeight );
         }
         resizeSurface( ctx, 0, 0, ctx->planeWidth, ctx->planeHeight);
      }
   }
}

static void outputDone( void *data, struct wl_output *output )
{
   UNUSED(data);
   UNUSED(output);
}

static void outputScale( void *data, struct wl_output *output, int32_t factor )
{
   UNUSED(data);
   UNUSED(output);
   UNUSED(factor);
}

static const struct wl_output_listener outputListener = {
   outputGeometry,
   outputMode,
   outputDone,
   outputScale
};

static void registryHandleGlobal(void *data, 
                                 struct wl_registry *registry, uint32_t id,
		                           const char *interface, uint32_t version)
{
	AppCtx *ctx = (AppCtx*)data;
	int len;

   printf("rne-triangle: registry: id %d interface (%s) version %d\n", id, interface, version );

   len= strlen(interface);
   if ( (len==6) && !strncmp(interface, "wl_shm", len)) {
      ctx->shm= (struct wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
      printf("shm %p\n", ctx->shm);
		wl_shm_add_listener(ctx->shm, &shmListener, ctx);
   }
   else if ( (len==13) && !strncmp(interface, "wl_compositor", len) ) {
      ctx->compositor= (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
      printf("compositor %p\n", ctx->compositor);
   } 
   else if ( (len==7) && !strncmp(interface, "wl_seat", len) ) {
      ctx->seat= (struct wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 4);
      printf("seat %p\n", ctx->seat);
		wl_seat_add_listener(ctx->seat, &seatListener, ctx);
   }
   else if ( (len==9) && !strncmp(interface, "wl_output", len) ) {
      ctx->output= (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
      printf("output %p\n", ctx->output);
      wl_output_add_listener(ctx->output, &outputListener, ctx);
   }
/*
   else if ( (len==15) && !strncmp(interface, "wl_simple_shell", len) ) {
      if ( ctx->getShell ) {
         ctx->shell= (struct wl_simple_shell*)wl_registry_bind(registry, id, &wl_simple_shell_interface, 1);      
         printf("shell %p\n", ctx->shell );
         wl_simple_shell_add_listener(ctx->shell, &shellListener, ctx);
      }
   }
*/
}

static void registryHandleGlobalRemove(void *data, 
                                       struct wl_registry *registry,
			                              uint32_t name)
{
}

static void shellSurfaceId(void *data,
                           struct wl_simple_shell *wl_simple_shell,
                           struct wl_surface *surface,
                           uint32_t surfaceId)
{
	AppCtx *ctx = (AppCtx*)data;
   char name[32];
  
	sprintf( name, "rne-triangle-surface-%x", surfaceId );
   printf("shell: surface created: %p id %x\n", surface, surfaceId);
	wl_simple_shell_set_name( ctx->shell, surfaceId, name );
}
                           
static void shellSurfaceCreated(void *data,
                                struct wl_simple_shell *wl_simple_shell,
                                uint32_t surfaceId,
                                const char *name)
{
	AppCtx *ctx = (AppCtx*)data;

   printf("shell: surface created: %x name: %s\n", surfaceId, name);
   ctx->surfaceIdOther= ctx->surfaceIdCurrent;
   ctx->surfaceIdCurrent= surfaceId;   
   wl_simple_shell_get_status( ctx->shell, ctx->surfaceIdCurrent );
   printf("shell: surfaceCurrent: %x surfaceOther: %x\n", ctx->surfaceIdCurrent, ctx->surfaceIdOther);
}

static void shellSurfaceDestroyed(void *data,
                                  struct wl_simple_shell *wl_simple_shell,
                                  uint32_t surfaceId,
                                  const char *name)
{
	AppCtx *ctx = (AppCtx*)data;
	
   printf("shell: surface destroyed: %x name: %s\n", surfaceId, name);
   
   if ( ctx->surfaceIdCurrent == surfaceId )
   {
      ctx->surfaceIdCurrent= ctx->surfaceIdOther;
      ctx->surfaceIdOther= 0;
      wl_simple_shell_get_status( ctx->shell, ctx->surfaceIdCurrent );
   }
   if ( ctx->surfaceIdOther == surfaceId )
   {
      ctx->surfaceIdOther= 0;
   }
   printf("shell: surfaceCurrent: %x surfaceOther: %x\n", ctx->surfaceIdCurrent, ctx->surfaceIdOther);
}
                                  
static void shellSurfaceStatus(void *data,
                               struct wl_simple_shell *wl_simple_shell,
                               uint32_t surfaceId,
                               const char *name,
                               uint32_t visible,
                               int32_t x,
                               int32_t y,
                               int32_t width,
                               int32_t height,
                               wl_fixed_t opacity,
                               wl_fixed_t zorder)
{
	AppCtx *ctx = (AppCtx*)data;

   printf("shell: surface: %x name: %s\n", surfaceId, name);
   printf("shell: position (%d,%d,%d,%d) visible %d opacity %f zorder %f\n",
           x, y, width, height, visible, wl_fixed_to_double(opacity), wl_fixed_to_double(zorder) );

   ctx->surfaceVisible= visible;
   ctx->surfaceX= x;
   ctx->surfaceY= y;
   ctx->surfaceWidth= width;
   ctx->surfaceHeight= height;
   ctx->surfaceOpacity= wl_fixed_to_double(opacity);
   ctx->surfaceZOrder= wl_fixed_to_double(zorder);   
}                               

static void shellGetSurfacesDone(void *data,
                                 struct wl_simple_shell *wl_simple_shell)
{
	//AppCtx *ctx = (AppCtx*)data;
	printf("shell: get all surfaces done\n");
}                                        

#define NON_BLOCKING_ENABLED (0)
#define NON_BLOCKING_DISABLED (1)

static void setBlockingMode(int blockingState )  
{  
   struct termios ttystate;
   int mask, bits;  
 
   mask= (blockingState == NON_BLOCKING_ENABLED) ? ~(ICANON|ECHO) : -1;
   bits= (blockingState == NON_BLOCKING_ENABLED) ? 0 : (ICANON|ECHO);

   // Obtain the current terminal state and alter the attributes to achieve 
   // the requested blocking behaviour
   tcgetattr(STDIN_FILENO, &ttystate);  

   ttystate.c_lflag= ((ttystate.c_lflag & mask) | bits);  
 
   if (blockingState == NON_BLOCKING_ENABLED)  
   {  
       ttystate.c_cc[VMIN]= 1;  
   }  

   tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);   
}


static void redraw( void *data, struct wl_callback *callback, uint32_t time )
{
   AppCtx *ctx= (AppCtx*)data;

   if ( g_log ) printf("redraw: time %u\n", time);
   wl_callback_destroy( callback );

   ctx->needRedraw= true;
}

static struct wl_callback_listener frameListener=
{
   redraw
};


int initilizeDisplay()
{
   int nRC= 0;
   AppCtx ctx;
   
   struct wl_display *display= 0;
   struct wl_registry *registry= 0;

   memset( &ctx, 0, sizeof(AppCtx) );

   const char * display_name = getDisplayEnv(); 
   if ( display_name)
   {
      printf("calling wl_display_connect for display name %s\n", display_name);
   }
   else
   {
      printf("calling wl_display_connect for default display\n");
   }
   display= wl_display_connect(display_name);
   printf("wl_display_connect: display=%p\n", display);
   if ( !display )
   {
      printf("error: unable to connect to primary display\n");
      nRC= -1;
      goto exit;
   }

   printf("calling wl_display_get_registry\n");
   registry= wl_display_get_registry(display);
   printf("wl_display_get_registry: registry=%p\n", registry);
   if ( !registry )
   {
      printf("error: unable to get display registry\n");
      nRC= -2;
      goto exit;
   }

   ctx.display= display;
   ctx.registry= registry;
   ctx.planeX= 0;
   ctx.planeY= 0;
   ctx.planeWidth= 1280;
   ctx.planeHeight= 720;
   wl_registry_add_listener(registry, &registryListener, &ctx);
   
   wl_display_roundtrip(ctx.display);
   
   setupEGL(&ctx);

   ctx.surfaceWidth= 1280;
   ctx.surfaceHeight= 720;
   ctx.surfaceX= 0;
   ctx.surfaceY= 0;

   createSurface(&ctx);
   
   requestFocusForApp();
   setupGL(&ctx);
   
  
	sigint.sa_handler = signalHandler;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sigint, NULL);

   setBlockingMode(NON_BLOCKING_ENABLED);

   ctx.inputState= InputState_main;
   ctx.attribute= Attribute_position;
   g_running= 1;
   while( g_running )
   {
      if ( wl_display_dispatch( ctx.display ) == -1 )
      {
         break;
      }

      if ( !paceRendering )
      {
         if ( delay > 0 )
         {
            usleep(delay);
         }
         renderGL(&ctx);
         eglSwapBuffers(ctx.eglDisplay, ctx.eglSurfaceWindow);
      }
   }   

exit:

   printf("rne_triangle: exiting...\n");

   setBlockingMode(NON_BLOCKING_DISABLED);
   
   if ( ctx.compositor )
   {
      wl_compositor_destroy( ctx.compositor );
      ctx.compositor= 0;
   }
   
   if ( ctx.shell )
   {
      wl_simple_shell_destroy( ctx.shell );
      ctx.shell= 0;
   }
   
   termEGL(&ctx);


   if ( ctx.seat )
   {
      wl_seat_destroy(ctx.seat);
      ctx.seat= 0;
   }

   if ( ctx.output )
   {
      wl_output_destroy(ctx.output);
      ctx.output= 0;
   }

   if ( registry )
   {
      wl_registry_destroy(registry);
      registry= 0;
   }
   
   if ( display )
   {
      wl_display_disconnect(display);
      display= 0;
   }
   
   printf("rne_triangle: exit\n");
      
   return nRC;
}

#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (0)

static bool setupEGL( AppCtx *ctx )
{
   bool result= false;
   EGLint major, minor;
   EGLBoolean b;
   EGLint configCount;
   EGLConfig *eglConfigs= 0;
   EGLint attr[32];
   EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
   EGLint ctxAttrib[3];
   int i;

   /*
    * Get default EGL display
    */
   ctx->eglDisplay = eglGetDisplay((NativeDisplayType)ctx->display);
   printf("eglDisplay=%p\n", ctx->eglDisplay );
   if ( ctx->eglDisplay == EGL_NO_DISPLAY )
   {
      printf("error: EGL not available\n" );
      goto exit;
   }
    
   /*
    * Initialize display
    */
   b= eglInitialize( ctx->eglDisplay, &major, &minor );
   if ( !b )
   {
      printf("error: unable to initialize EGL display\n" );
      goto exit;
   }
   printf("eglInitiialize: major: %d minor: %d\n", major, minor );
    
   /*
    * Get number of available configurations
    */
   b= eglGetConfigs( ctx->eglDisplay, NULL, 0, &configCount );
   if ( !b )
   {
      printf("error: unable to get count of EGL configurations: %X\n", eglGetError() );
      goto exit;
   }
   printf("Number of EGL configurations: %d\n", configCount );
    
   eglConfigs= (EGLConfig*)malloc( configCount*sizeof(EGLConfig) );
   if ( !eglConfigs )
   {
      printf("error: unable to alloc memory for EGL configurations\n");
      goto exit;
   }
    
   i= 0;
   attr[i++]= EGL_RED_SIZE;
   attr[i++]= RED_SIZE;
   attr[i++]= EGL_GREEN_SIZE;
   attr[i++]= GREEN_SIZE;
   attr[i++]= EGL_BLUE_SIZE;
   attr[i++]= BLUE_SIZE;
   attr[i++]= EGL_DEPTH_SIZE;
   attr[i++]= DEPTH_SIZE;
   attr[i++]= EGL_STENCIL_SIZE;
   attr[i++]= 0;
   attr[i++]= EGL_SURFACE_TYPE;
   attr[i++]= EGL_WINDOW_BIT;
   attr[i++]= EGL_RENDERABLE_TYPE;
   attr[i++]= EGL_OPENGL_ES2_BIT;
   attr[i++]= EGL_NONE;
    
   /*
    * Get a list of configurations that meet or exceed our requirements
    */
   b= eglChooseConfig( ctx->eglDisplay, attr, eglConfigs, configCount, &configCount );
   if ( !b )
   {
      printf("error: eglChooseConfig failed: %X\n", eglGetError() );
      goto exit;
   }
   printf("eglChooseConfig: matching configurations: %d\n", configCount );
    
   /*
    * Choose a suitable configuration
    */
   for( i= 0; i < configCount; ++i )
   {
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_RED_SIZE, &redSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_GREEN_SIZE, &greenSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_BLUE_SIZE, &blueSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_ALPHA_SIZE, &alphaSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_DEPTH_SIZE, &depthSize );

      printf("config %d: red: %d green: %d blue: %d alpha: %d depth: %d\n",
              i, redSize, greenSize, blueSize, alphaSize, depthSize );
      if ( (redSize == RED_SIZE) &&
           (greenSize == GREEN_SIZE) &&
           (blueSize == BLUE_SIZE) &&
           (alphaSize == ALPHA_SIZE) &&
           (depthSize >= DEPTH_SIZE) )
      {
         printf( "choosing config %d\n", i);
         break;
      }
   }
   if ( i == configCount )
   {
      printf("error: no suitable configuration available\n");
      goto exit;
   }
   ctx->eglConfig= eglConfigs[i];

   ctxAttrib[0]= EGL_CONTEXT_CLIENT_VERSION;
   ctxAttrib[1]= 2; // ES2
   ctxAttrib[2]= EGL_NONE;
    
   /*
    * Create an EGL context
    */
   ctx->eglContext= eglCreateContext( ctx->eglDisplay, ctx->eglConfig, EGL_NO_CONTEXT, ctxAttrib );
   if ( ctx->eglContext == EGL_NO_CONTEXT )
   {
      printf( "eglCreateContext failed: %X\n", eglGetError() );
      goto exit;
   }
   printf("eglCreateContext: eglContext %p\n", ctx->eglContext );

   result= true;
    
exit:

   if ( eglConfigs )
   {
      free( eglConfigs );
      eglConfigs= 0;
   }

   return result;       
}

static void termEGL( AppCtx *ctx )
{
   if ( ctx->display )
   {
      eglMakeCurrent( ctx->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
      
      destroySurface( ctx );
      
      eglTerminate( ctx->eglDisplay );
      eglReleaseThread();
   }
}

static bool createSurface( AppCtx *ctx )
{
   bool result= false;
   EGLBoolean b;

   ctx->surface= wl_compositor_create_surface(ctx->compositor);
   printf("surface=%p\n", ctx->surface);   
   if ( !ctx->surface )
   {
      printf("error: unable to create wayland surface\n");
      goto exit;
   }

   ctx->native= wl_egl_window_create(ctx->surface, ctx->planeWidth, ctx->planeHeight);
   if ( !ctx->native )
   {
      printf("error: unable to create wl_egl_window\n");
      goto exit;
   }
   printf("wl_egl_window %p\n", ctx->native);
   
   /*
    * Create a window surface
    */
   ctx->eglSurfaceWindow= eglCreateWindowSurface( ctx->eglDisplay,
                                                  ctx->eglConfig,
                                                  (EGLNativeWindowType)ctx->native,
                                                  NULL );
   if ( ctx->eglSurfaceWindow == EGL_NO_SURFACE )
   {
      printf("eglCreateWindowSurface: A: error %X\n", eglGetError() );
      ctx->eglSurfaceWindow= eglCreateWindowSurface( ctx->eglDisplay,
                                                     ctx->eglConfig,
                                                     (EGLNativeWindowType)NULL,
                                                     NULL );
      if ( ctx->eglSurfaceWindow == EGL_NO_SURFACE )
      {
         printf("eglCreateWindowSurface: B: error %X\n", eglGetError() );
         goto exit;
      }
   }
   printf("eglCreateWindowSurface: eglSurfaceWindow %p\n", ctx->eglSurfaceWindow );                                         

   /*
    * Establish EGL context for this thread
    */
   b= eglMakeCurrent( ctx->eglDisplay, ctx->eglSurfaceWindow, ctx->eglSurfaceWindow, ctx->eglContext );
   if ( !b )
   {
      printf("error: eglMakeCurrent failed: %X\n", eglGetError() );
      goto exit;
   }
    
   eglSwapInterval( ctx->eglDisplay, 1 );

exit:

   return result;
}

static void destroySurface( AppCtx *ctx )
{
   if ( ctx->eglSurfaceWindow )
   {
      eglDestroySurface( ctx->eglDisplay, ctx->eglSurfaceWindow );
      
      wl_egl_window_destroy( ctx->native );   
   }
   if ( ctx->surface )
   {
      wl_surface_destroy( ctx->surface );
      ctx->surface= 0;
   }
}

static void resizeSurface( AppCtx *ctx, int dx, int dy, int width, int height )
{
   wl_egl_window_resize( ctx->native, width, height, dx, dy );
}

static const char *vert_shader_text =
	"uniform mat4 rotation;\n"
	"attribute vec4 pos;\n"
	"attribute vec4 color;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_Position = rotation * pos;\n"
	"  v_color = color;\n"
	"}\n";

static const char *frag_shader_text =
	"precision mediump float;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_FragColor = v_color;\n"
	"}\n";

static GLuint createShader(AppCtx *ctx, GLenum shaderType, const char *shaderSource )
{
   GLuint shader= 0;
   GLint shaderStatus;
   GLsizei length;
   char logText[1000];
   
   shader= glCreateShader( shaderType );
   if ( shader )
   {
      glShaderSource( shader, 1, (const char **)&shaderSource, NULL );
      glCompileShader( shader );
      glGetShaderiv( shader, GL_COMPILE_STATUS, &shaderStatus );
      if ( !shaderStatus )
      {
         glGetShaderInfoLog( shader, sizeof(logText), &length, logText );
         printf("Error compiling %s shader: %*s\n",
                ((shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment"),
                length,
                logText );
      }
   }
   
   return shader;
}

static bool setupGL( AppCtx *ctx )
{
   bool result= false;
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag= createShader(ctx, GL_FRAGMENT_SHADER, frag_shader_text);
	vert= createShader(ctx, GL_VERTEX_SHADER, vert_shader_text);

	program= glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) 
	{
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		goto exit;
	}

	glUseProgram(program);

	ctx->gl.pos= 0;
	ctx->gl.col= 1;

	glBindAttribLocation(program, ctx->gl.pos, "pos");
	glBindAttribLocation(program, ctx->gl.col, "color");
	glLinkProgram(program);

	ctx->gl.rotation_uniform= glGetUniformLocation(program, "rotation");
		
exit:
   return result;		
}

