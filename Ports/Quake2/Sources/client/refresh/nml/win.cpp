#include "../win_platform.h"
#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <netgl/netgl.hpp>

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
    return true;
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
    return false;
}