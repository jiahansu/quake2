#include "../win_platform.h"
#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <netgl/netgl.hpp>
#include <netml/netml.hpp>

extern "C"{
#include <OpenGLES/OpenGLWrapper.h>
#include <client/refresh/r_private.h>
}

using namespace netffi;
using namespace ngl;

extern "C" bool R_Window_setup()
{
    return false;
}

bool R_Window_createContext()
{
	SdlwContext *sdlw = sdlwContext;
    IClient* pClient = reinterpret_cast<IClient*>(sdlw->pNGLClient);
    nglMakeCurrent(*pClient);

	if (oglwCreate()){
		return false;
	}else{
		return true;
	}
}

void R_Window_finalize()
{
	SDL_ShowCursor(1);

	In_Grab(false);

	/* Clear the backbuffer and make it
	   current. This may help some broken
	   video drivers like the AMD Catalyst
	   to avoid artifacts in unused screen
	   areas. */
	if (oglwIsCreated())
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		oglwEnableDepthWrite(false);
		glStencilMask(0xffffffff);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepthf(1.0f);
		glClearStencil(0);
		oglwClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		R_Frame_end();

		oglwDestroy();
	}

	//eglwFinalize();
	sdlwDestroyWindow();

	gl_state.hwgamma = false;
}

bool R_Window_update(bool forceFlag){
	SdlwContext *sdlw = sdlwContext;
	bool updateNeeded = true;
	if (sdlw->pWindow == NULL)
        {
            char windowName[64];
            snprintf(windowName, sizeof(windowName), QUAKE2_COMPLETE_NAME);
            windowName[63] = 0;
           
			//R_printf(PRINT_ALL, "Creating a window with width=%i height=%i fullscreen=%i\n", creationWidth, creationHeight, fullscreen);
            if (!sdlwCreateWindow(windowName, 0, 0, 0))
            {
                updateNeeded = true;
            }
            else
            {
				std::cerr<<"Cannot create the window"<<std::endl;
			}
        }

		if (updateNeeded)
            {
				int windowX = 0, windowY = 0;

				Cvar_SetValue("r_window_width", sdlw->windowWidth);
				Cvar_SetValue("r_window_height", sdlw->windowHeight);
				Cvar_SetValue("r_window_x", windowX);
				Cvar_SetValue("r_window_y", windowY);
				Cvar_SetValue("r_fullscreen", false);
				
				if (R_Window_setup())
					R_printf(PRINT_ALL, "Failed to update the window.\n");
			}

	viddef.width = sdlw->windowWidth;
	viddef.height = sdlw->windowHeight;
    return false;
}

static void Gles_checkGlesError() 
{
	GLenum error;
	while( (error = glGetError()) != GL_NO_ERROR )
		std::cerr<<"GLES Error: "<<static_cast<int32_t>(error)<<std::endl;
}

void R_Frame_end()
{
	// #ifdef SAILFISH_FBO
		// static unsigned long long frame_Count = 0;
	// #endif
	SdlwContext *sdlw = sdlwContext;
    NMLWinHandler* pHandler = reinterpret_cast<NMLWinHandler*>(sdlw->pWindow);
	if (r_discardframebuffer->value && gl_config.discardFramebuffer)
	{
		static const GLenum attachements[] = { GL_DEPTH_EXT, GL_STENCIL_EXT };
        #if defined(EGLW_GLES2)
		gl_config.discardFramebuffer(GL_FRAMEBUFFER, 2, attachements);
        #else
		gl_config.discardFramebuffer(GL_FRAMEBUFFER_OES, 2, attachements);
        #endif
	}


	nglSwapBuffers(pHandler->winId);
	Gles_checkGlesError();
}