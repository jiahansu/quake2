#include "../win_platform.h"

#include "client/client.h"
#include "client/keyboard.h"
#include "client/refresh/r_private.h"

static bool R_Window_getNearestDisplayMode(SDL_DisplayMode *nearestMode)
{
    int requestedWidth = r_fullscreen_width->value;
    int requestedHeight = r_fullscreen_height->value;
    int requestedBpp = r_fullscreen_bitsPerPixel->value;
    int requestedFrequency = r_fullscreen_frequency->value;

    if (requestedWidth < R_WIDTH_MIN)
        requestedWidth = R_WIDTH_MIN;
    if (requestedHeight < R_HEIGHT_MIN)
        requestedHeight = R_HEIGHT_MIN;
    if (requestedBpp < 15)
        requestedBpp = 15;
    if (requestedFrequency < 0)
        requestedFrequency = 0;

    int displayModeNb = SDL_GetNumDisplayModes(0);
    if (displayModeNb < 1)
    {
        R_printf(PRINT_ALL, "SDL_GetNumDisplayModes failed: %s\n", SDL_GetError());
        return true;
    }
    R_printf(PRINT_ALL, "SDL_GetNumDisplayModes: %i\n", displayModeNb);

    SDL_DisplayMode bestMode;
    int bestModeIndex = -1;
    for (int i = 0; i < displayModeNb; ++i) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) != 0)
        {
            R_printf(PRINT_ALL, "SDL_GetDisplayMode %i failed: %s\n", i, SDL_GetError());
            break;
        }
        R_printf(PRINT_ALL, "Mode %i %ix%i %i bpp (%s) %i Hz\n", i, mode.w, mode.h, SDL_BITSPERPIXEL(mode.format), SDL_GetPixelFormatName(mode.format), mode.refresh_rate);
        bool nearer =
            abs(requestedWidth - mode.w) <= abs(requestedWidth - bestMode.w) && abs(requestedHeight - mode.h) <= abs(requestedHeight - bestMode.h) &&
            abs(requestedBpp - (int)SDL_BITSPERPIXEL(mode.format)) <= abs(requestedBpp - (int)SDL_BITSPERPIXEL(bestMode.format)) &&
            abs(requestedFrequency - mode.refresh_rate) <= abs(requestedFrequency - bestMode.refresh_rate)
            ;
        bool bestAbove = bestMode.w >= requestedWidth && bestMode.h >= requestedHeight && (int)SDL_BITSPERPIXEL(bestMode.format) >= requestedBpp && bestMode.refresh_rate >= requestedFrequency;
        bool above = mode.w >= requestedWidth && mode.h >= requestedHeight && (int)SDL_BITSPERPIXEL(mode.format) >= requestedBpp && mode.refresh_rate >= requestedFrequency;
        if ((above && !bestAbove) || nearer)
        {
            bestMode = mode;
            bestModeIndex = i;
        }
    }
    if (bestModeIndex < 0)
        return true;
    *nearestMode = bestMode;
    return false;
}

bool R_Window_setup()
{
	SdlwContext *sdlw = sdlwContext;
    bool fullscreen = r_fullscreen->value;
    if (fullscreen)
    {
        SDL_DisplayMode displayMode;
        if (R_Window_getNearestDisplayMode(&displayMode))
            return true;
		R_printf(PRINT_ALL, "Updating window with width=%i height=%i fullscreen=1 bpp=%i frequency=%i\n", displayMode.w, displayMode.h, (int)SDL_BITSPERPIXEL(displayMode.format), displayMode.refresh_rate);
        if (SDL_SetWindowDisplayMode(sdlw->window, &displayMode) != 0)
            return true;
        SDL_SetWindowPosition(sdlw->window, 0, 0);
        SDL_SetWindowSize(sdlw->window, displayMode.w, displayMode.h); // Must be after SDL_SetWindowDisplayMode() (SDL bug ?).
        if (SDL_SetWindowFullscreen(sdlw->window, SDL_WINDOW_FULLSCREEN) != 0)  // This must be after SDL_SetWindowPosition() and SDL_SetWindowSize() when fullscreen is enabled !
            return true;
    }
    else
    {
        int windowWidth = r_window_width->value, windowHeight = r_window_height->value;
		R_printf(PRINT_ALL, "Updating window with width=%i height=%i fullscreen=0\n", windowWidth, windowHeight);
        if (SDL_SetWindowFullscreen(sdlw->window, 0) != 0) // This must be before SDL_SetWindowPosition() and SDL_SetWindowSize() when fullscreen is disabled !
            return true;
        int windowX = r_window_x->value, windowY = r_window_y->value;
        SDL_SetWindowPosition(sdlw->window, windowX, windowY);
        SDL_SetWindowSize(sdlw->window, windowWidth, windowHeight);
    }
    return false;
}

static void R_Gamma_initialize()
{
	#if defined(HARDWARE_GAMMA_ENABLED)
	R_printf(PRINT_ALL, "Using hardware gamma via SDL.\n");
	#endif
	gl_state.hwgamma = true;
	r_gamma->modified = true;
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

	eglwFinalize();
	sdlwDestroyWindow();

	gl_state.hwgamma = false;
}

bool R_Window_createContext()
{
	while (1)
	{
		EglwConfigInfo cfgiMinimal;
		cfgiMinimal.redSize = 5; cfgiMinimal.greenSize = 5; cfgiMinimal.blueSize = 5; cfgiMinimal.alphaSize = 0;
		cfgiMinimal.depthSize = 16; cfgiMinimal.stencilSize = 0; cfgiMinimal.samples = 0;
		EglwConfigInfo cfgiRequested;
		cfgiRequested.redSize = 5; cfgiRequested.greenSize = 5; cfgiRequested.blueSize = 5; cfgiRequested.alphaSize = 0;
		cfgiRequested.depthSize = 16; cfgiRequested.stencilSize = 1; cfgiRequested.samples = (int)r_msaa_samples->value;

		if (eglwInitialize(&cfgiMinimal, &cfgiRequested, false))
		{
            R_printf(PRINT_ALL, "Cannot create an OpenGL ES context.\n");
			if (r_msaa_samples->value)
			{
				R_printf(PRINT_ALL, "Trying without MSAA.\n");
				Cvar_SetValue("r_msaa_samples", 0);
			}
			else
			{
				R_printf(PRINT_ALL, "All attempts failed.\n");
				goto on_error;
			}
		}
		else
		{
			break;
		}
	}

#ifdef SAILFISH_FBO
	Cvar_SetValue("r_sizerender", 0.5);
	create_fbo(viddef.width, viddef.height);
#endif

	r_msaaAvailable = (eglwContext->configInfoAbilities.samples > 0);
	Cvar_SetValue("r_msaa_samples", eglwContext->configInfo.samples);
	r_stencilAvailable = (eglwContext->configInfo.stencilSize > 0);

	eglSwapInterval(eglwContext->display, gl_swapinterval->value ? 1 : 0);

	if (oglwCreate())
		goto on_error;

	R_Gamma_initialize();
#ifdef SAILFISH_FBO
	SDL_ShowCursor(1);
#else
	SDL_ShowCursor(0);
#endif
	return true;

on_error:
	R_Window_finalize();
	return false;
}

static void R_Window_getValidWindowSize(int maxWindowWidth, int maxWindowHeight, int requestedWidth, int requestedHeight, int *windowWidth, int *windowHeight)
{
	#if defined(__GCW_ZERO__)
	(void)maxWindowWidth;
	(void)maxWindowHeight;
	(void)requestedWidth;
	(void)requestedHeight;
    *windowWidth = 320;
    *windowHeight = 240;
	#else
    if (maxWindowWidth > 0 && requestedWidth > maxWindowWidth)
        requestedWidth = maxWindowWidth;
    if (maxWindowHeight > 0 && requestedHeight > maxWindowHeight)
        requestedHeight = maxWindowHeight;
    if (requestedWidth < R_WIDTH_MIN)
        requestedWidth = R_WIDTH_MIN;
    if (requestedHeight < R_HEIGHT_MIN)
        requestedHeight = R_HEIGHT_MIN;
#if defined(SAILFISH_FBO) && defined(SAILFISHOS)
    requestedHeight = maxWindowWidth;
    requestedWidth = maxWindowHeight;
#endif
    *windowWidth = requestedWidth;
    *windowHeight = requestedHeight;
    #endif
}

static void R_Window_getMaxWindowSize(int *windowWidth, int *windowHeight)
{
	#if defined(__GCW_ZERO__)
    *windowWidth = 320;
    *windowHeight = 240;
	#else
    int maxWindowWidth = 0, maxWindowHeight = 0;
    #if SDL_VERSION_ATLEAST(2, 0, 5)
    SDL_Rect rect;
    if (SDL_GetDisplayUsableBounds(0, &rect) == 0)
    {
        maxWindowWidth = rect.w;
        maxWindowHeight = rect.h;
    }
    else
    #endif
    {
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(0, &mode) == 0)
        {
            maxWindowWidth = mode.w;
            maxWindowHeight = mode.h;
        }
    }   
    *windowWidth = maxWindowWidth;
    *windowHeight = maxWindowHeight;
    #endif
}

//********************************************************************************
// Window.
//********************************************************************************
//--------------------------------------------------------------------------------
// Icon.
//--------------------------------------------------------------------------------
// The 64x64 32bit window icon.
#include "backends/sdl/icon/q2icon64.h"

static void R_Window_setIcon()
{
	/* these masks are needed to tell SDL_CreateRGBSurface(From)
	   to assume the data it gets is byte-wise RGB(A) data */
	Uint32 rmask, gmask, bmask, amask;
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (q2icon64.bytes_per_pixel == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
	#else /* little endian, like x86 */
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (q2icon64.bytes_per_pixel == 3) ? 0 : 0xff000000;
	#endif

	SDL_Surface * icon = SDL_CreateRGBSurfaceFrom((void *)q2icon64.pixel_data, q2icon64.width,
			q2icon64.height, q2icon64.bytes_per_pixel * 8, q2icon64.bytes_per_pixel * q2icon64.width,
			rmask, gmask, bmask, amask);

	SDL_SetWindowIcon(sdlwContext->window, icon);

	SDL_FreeSurface(icon);
}

bool R_Window_update(bool forceFlag)
{
	SdlwContext *sdlw = sdlwContext;

    int maxWindowWidth, maxWindowHeight;
    R_Window_getMaxWindowSize(&maxWindowWidth, &maxWindowHeight);
    
    bool exitFlag = false;
    while (1)
    {
        int windowWidth, windowHeight;
        R_Window_getValidWindowSize(maxWindowWidth, maxWindowHeight, r_window_width->value, r_window_height->value, &windowWidth, &windowHeight);

        bool updateNeeded = false;

        #if defined(R_WINDOWED_MODE_DISABLED) || (defined(SAILFISHOS) && defined(SAILFISH_FBO))
        bool fullscreen = true;
        #else
        bool fullscreen = r_fullscreen->value;
		#endif

        if (sdlw->window == NULL)
        {
            char windowName[64];
            snprintf(windowName, sizeof(windowName), QUAKE2_COMPLETE_NAME);
            windowName[63] = 0;
            int creationWidth = windowWidth, creationHeight = windowHeight;
            #if defined(R_WINDOWED_MODE_DISABLED)
            int flags = 0;
            #else
            int flags = SDL_WINDOW_RESIZABLE;
            #endif
			#if !defined(__RASPBERRY_PI__)
            if (fullscreen)
				flags |= SDL_WINDOW_FULLSCREEN;
			#endif
			#if defined(SAILFISH_FBO) && defined(SAILFISHOS)
				creationWidth = windowHeight;
				creationHeight = windowWidth;
			#endif
			R_printf(PRINT_ALL, "Creating a window with width=%i height=%i fullscreen=%i\n", creationWidth, creationHeight, fullscreen);
            if (!sdlwCreateWindow(windowName, creationWidth, creationHeight, flags))
            {
                R_Window_setIcon();
				#if defined(__RASPBERRY_PI__)
				r_window_width->modified = false;
				r_window_height->modified = false;
				#elif defined(SAILFISHOS)
				r_window_width->modified = false;
				r_window_height->modified = false;
				#else
                updateNeeded = true;
                #endif
            }
            else
            {
				R_printf(PRINT_ALL, "Cannot create the window\n");
			}
        }

        if (sdlw->window != NULL)
        {
			#if defined(__GCW_ZERO__)
			exitFlag = true;
            #elif defined(__RASPBERRY_PI__)
			if (r_window_width->modified || r_window_height->modified)
                updateNeeded = true;
            if (updateNeeded)
            {
				// For Raspberry Pi, we cannot change the size of the framebuffer dynamically. So restart the rendering system.
				R_printf(PRINT_ALL, "Restarting rendering system.\n");
				R_restart();
			}
			exitFlag = true;
			#else
            bool currentFullscreen = (SDL_GetWindowFlags(sdlw->window) & SDL_WINDOW_FULLSCREEN) != 0;
            if (fullscreen != currentFullscreen)
            {
                updateNeeded = true;
                if (!r_fullscreen->modified)
                {
                    // The fullscreen state has been changed externally (by the user or the system).
                    fullscreen = currentFullscreen;
                }
                else
                {
                    // The fullscreen state has been changed internally (in the menus or via the console).
                }
            }
            if (fullscreen)
            {
                if (r_fullscreen_width->modified || r_fullscreen_height->modified || r_fullscreen_bitsPerPixel->modified || r_fullscreen_frequency->modified)
                    updateNeeded = true;
            }

            if (!currentFullscreen)
            {
                int currentWidth, currentHeight;
			#if defined(SAILFISH_FBO) && defined(SAILFISHOS)
				SDL_GetWindowSize(sdlw->window, &currentHeight, &currentWidth);
			#else
                SDL_GetWindowSize(sdlw->window, &currentWidth, &currentHeight);
			#endif
                if (windowWidth != currentWidth || windowHeight != currentHeight)
                {
                    updateNeeded = true;
                    if (!r_window_width->modified && !r_window_height->modified)
                    {
                        // The window size has been changed externally (by the user or the system).
                        R_Window_getValidWindowSize(maxWindowWidth, maxWindowHeight, currentWidth, currentHeight, &windowWidth, &windowHeight);
                    }
                    else
                    {
                        // The window size has been changed internally (in the menus or via the console).
                    }
                }
            }

            if (updateNeeded)
            {
				int windowX, windowY;
				if (currentFullscreen)
				{
					windowX = r_window_x->value;
					windowY = r_window_y->value;
				}
				else
				{
					SDL_GetWindowPosition(sdlw->window, &windowX, &windowY);
				}
				#if 0
				if (windowX > maxWindowWidth - windowWidth)
					windowX = maxWindowWidth - windowWidth;
				if (windowX < 0)
					windowX = 0;
				if (windowY > maxWindowHeight - windowHeight)
					windowY = maxWindowHeight - windowHeight;
				if (windowY < 0)
					windowY = 0;
				#endif

				Cvar_SetValue("r_window_width", windowWidth);
				Cvar_SetValue("r_window_height", windowHeight);
				Cvar_SetValue("r_window_x", windowX);
				Cvar_SetValue("r_window_y", windowY);
				Cvar_SetValue("r_fullscreen", fullscreen);
				
				if (R_Window_setup())
					R_printf(PRINT_ALL, "Failed to update the window.\n");
				else
					exitFlag = true;
			}
            else
                exitFlag = true;
            #endif
        }
        
		r_window_width->modified = false;
		r_window_height->modified = false;
		r_window_x->modified = false;
		r_window_y->modified = false;
		r_fullscreen->modified = false;
		r_fullscreen_width->modified = false;
		r_fullscreen_height->modified = false;
		r_fullscreen_bitsPerPixel->modified = false;
		r_fullscreen_frequency->modified = false;

		if (exitFlag)
			break;

		#if !defined(R_WINDOWED_MODE_DISABLED)
        if (fullscreen)
        {
            R_printf(PRINT_ALL, "Trying with fullscreen = %i.\n", false);
            Cvar_SetValue("r_fullscreen", false);
        }
        else
		#endif
        {
            R_printf(PRINT_ALL, "All attempts failed\n");
            return true;
        }
    }
    
	int effectiveWidth, effectiveHeight;
#if defined(SAILFISH_FBO) && defined(SAILFISHOS)
	SDL_GetWindowSize(sdlw->window, &effectiveHeight, &effectiveWidth);
#else
	SDL_GetWindowSize(sdlw->window, &effectiveWidth, &effectiveHeight);
#endif
#if defined(SAILFISH_FBO)
	if( sailfish_fbo.bw > 0 && sailfish_fbo.bw > 0 ) {
		viddef.width = sailfish_fbo.bw;
		viddef.height = sailfish_fbo.bh;
	} else {
		viddef.width = effectiveWidth;
		viddef.height = effectiveHeight;	
	}
#else
	viddef.width = effectiveWidth;
	viddef.height = effectiveHeight;
#endif
	sdlwResize(effectiveWidth, effectiveHeight);
    return false;
}