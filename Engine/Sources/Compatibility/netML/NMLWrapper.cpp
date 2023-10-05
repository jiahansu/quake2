extern "C"{
#include "../SDL/SDLWrapper.h"
}

#include <stdio.h>
#include <stdlib.h>

#include <netffi/netffi.hpp>
#include <lan2p/lan2p.hpp>
#include <netffi/client.hpp>
#include <memory>
#include <vxo_ostream/slog.hpp>
#include <netgl/netgl.hpp>
#include <netml/netml.hpp>
#include <thread>
#include <gmi_event.hpp>
#include <mutex>
#include <queue>

SdlwContext *sdlwContext = NULL;

#ifdef SAILFISHOS
#include <SDL_hints.h>
#include <SDL_events.h>
#include <SDL_video.h>
#include <SDL_syswm.h>
#include <wayland-client-protocol.h>
#endif
//SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK

static auto LOG = VXO::Log::getLogger("NMLWrapper");

using namespace lan2p;
using namespace netffi;
using namespace netml;

static void sdlwLogOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    const char *categoryString = "";
    switch (category)
    {
    default:
        categoryString = "unknown";
        break;
    case SDL_LOG_CATEGORY_APPLICATION:
        categoryString = "application";
        break;
    case SDL_LOG_CATEGORY_ERROR:
        categoryString = "error";
        break;
    case SDL_LOG_CATEGORY_ASSERT:
        categoryString = "assert";
        break;
    case SDL_LOG_CATEGORY_SYSTEM:
        categoryString = "system";
        break;
    case SDL_LOG_CATEGORY_AUDIO:
        categoryString = "audio";
        break;
    case SDL_LOG_CATEGORY_VIDEO:
        categoryString = "video";
        break;
    case SDL_LOG_CATEGORY_RENDER:
        categoryString = "render";
        break;
    case SDL_LOG_CATEGORY_INPUT:
        categoryString = "input";
        break;
    case SDL_LOG_CATEGORY_TEST:
        categoryString = "test";
        break;
    }

    const char *priorityString = "unknown";
    switch (priority)
    {
    default:
        priorityString = "unknown";
        break;
    case SDL_LOG_PRIORITY_VERBOSE:
        priorityString = "verbose";
        break;
    case SDL_LOG_PRIORITY_DEBUG:
        priorityString = "debug";
        break;
    case SDL_LOG_PRIORITY_INFO:
        priorityString = "info";
        break;
    case SDL_LOG_PRIORITY_WARN:
        priorityString = "warn";
        break;
    case SDL_LOG_PRIORITY_ERROR:
        priorityString = "error";
        break;
    case SDL_LOG_PRIORITY_CRITICAL:
        priorityString = "critical";
        break;
    }
    
    printf("SDL - %s - %s - %s", categoryString, priorityString, message);
}

extern "C" bool sdlwInitialize(SdlProcessEventFunction processEvent, Uint32 flags) {
    IPeer* pPeer;
    IClient* pNGLClient;
    IClient* pNMLClient;
    ISession* pSession;
    struct sockaddr_in dst;
    std::string ip = "127.0.0.1";
    uint16_t port = 2409;
    bool b  = false;
    sdlwFinalize();
    
	SdlwContext *sdlw = reinterpret_cast<SdlwContext*>(malloc(sizeof(SdlwContext)));
	if (sdlw == NULL) return true;
	sdlwContext = sdlw;
    sdlw->exitRequested = false;
    sdlw->defaultEventManagementEnabled = true;
	sdlw->processEvent = processEvent;
    sdlw->pWindow = nullptr;
    sdlw->window = NULL;
    sdlw->windowWidth = 0;
    sdlw->windowHeight = 0;
    sdlw->pPeer = pPeer = lan2p::createPeer();
    sdlw->pNGLClient = pNGLClient = netffi::createClient();
    sdlw->pNMLClient = pNMLClient = netffi::createClient();
    sdlw->pEventMutex = new std::mutex();
    sdlw->pEventQueue = new std::queue<GMI::Event>();

#ifdef SAILFISHOS
    sdlw->orientation = SDL_ORIENTATION_LANDSCAPE;
#endif
    
    if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return true;
    }
    
    SDL_LogSetOutputFunction(sdlwLogOutputFunction, NULL);
//    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Test\n");
    
//    if (SDL_NumJoysticks() > 0) SDL_JoystickOpen(0);

    SLOG_INFO<<"Dst address: "<<ip<<':'<<port<<std::endl;

            dst.sin_family = AF_INET;
            dst.sin_addr.s_addr = inet_addr(ip.c_str());
            dst.sin_port = htons(port);

            pPeer->start(0);
            pPeer->buildSession(dst);

            //cv.wait_for(lock, std::chrono::seconds(15));
            pSession = pPeer->waitSessionOpened();
            if(pSession!=nullptr){
                pNGLClient->input() = &(pSession->channel(ngl::nglChannelId()).input());
                pNGLClient->output() = &(pSession->channel(ngl::nglChannelId()).output());
                pNMLClient->input() = &(pSession->channel(nmlChannelId()).input());
                pNMLClient->output() = &(pSession->channel(nmlChannelId()).output());
                b = true;
            }
            if(b){
                sdlw->pEventThread = new std::thread([pPeer, sdlw](){
                    gw::IBuffer& input = pPeer->session()->channel(2).input();
                    GMI::Event event = {};
                    std::mutex* pMutex = reinterpret_cast<std::mutex*>(sdlw->pEventMutex);
                    std::queue<GMI::Event>* pEventQueue = reinterpret_cast<std::queue<GMI::Event>*>(sdlw->pEventQueue);
                    SLOG_INFO<<"Start reveiving event loop..."<<std::endl;
                    while(!sdlw->exitRequested){
                        input>>event;
                        if(input.isAlive()){
                            const std::lock_guard lock(*pMutex);

                            pEventQueue->push(event);
                            //this->updateByEvents(event);
                        }
                        //SLOG_INFO<<"Receive event: "<<static_cast<uint32_t>(event.type)<<std::endl;
                    }
                    SLOG_INFO<<"End reveiving event loop..."<<std::endl;
                });
                /*
                m_Running = true;

                m_pEventThread = std::make_unique<std::thread>([this](){
                    gw::IBuffer& input = this->m_pNetPeer->session()->channel(EVENT_CHANNEL).input();
                    Event event = {};

                    while(m_Running){
                        input>>event;
                        if(input.isAlive()){
                            this->updateByEvents(event);
                        }
                        //SLOG_INFO<<"Receive event: "<<static_cast<uint32_t>(event.type)<<std::endl;
                    }
                });
                makeCurrentGLContext();
                m_WinHandler = nmlWindowId(m_pNMLClient.get());*/
            }
    
    return false;
    /*
	sdlwFinalize();
    return true;*/
}

extern "C" void sdlwFinalize() {
	SdlwContext *sdlw = sdlwContext;
	if (sdlw == NULL) return;
    
    SDL_Quit();

    IPeer* pPeer =  reinterpret_cast<IPeer*>(sdlw->pPeer);
    IClient* pNGLClient = reinterpret_cast<IClient*>(sdlw->pNGLClient);
    IClient* pNMLClient = reinterpret_cast<IClient*>(sdlw->pNMLClient);

    sdlw->exitRequested = true;

    if(sdlw->pEventThread!=nullptr){
        std::thread* pThread = reinterpret_cast<std::thread*>(sdlw->pEventThread);

        if(pThread->joinable()){
            pThread->join();
        }
    }

    if(pPeer!=nullptr){
        pPeer->stop();
        delete pPeer;
    }
    
    if(pNGLClient!=nullptr){
        delete pNGLClient;
    }

    if(pNMLClient!=nullptr){
        delete pNMLClient;
    }

    delete reinterpret_cast<std::mutex*>(sdlw->pEventMutex);
    delete reinterpret_cast<std::queue<GMI::Event>*>(sdlw->pEventQueue);
    delete reinterpret_cast<NMLWinHandler*>(sdlw->pWindow);
    
    
    free(sdlw);
    sdlwContext = NULL;
}

extern "C" bool sdlwCreateWindow(const char *windowName, int windowWidth, int windowHeight, Uint32 flags)
{
	SdlwContext *sdlw = sdlwContext;
	if (sdlw == NULL) return true;

    sdlwDestroyWindow();

    NMLWinHandler handler  = nmlWindowId(sdlw->pNMLClient);
    NMLWinHandler* pHandler = new NMLWinHandler();

    pHandler->pClient = reinterpret_cast<IClient*>(sdlw->pNMLClient);
    pHandler->winId = handler.winId;

    sdlw->pWindow = pHandler;

    nmlWinSize(*pHandler,&(sdlw->windowWidth), &(sdlw->windowHeight));

    //int windowPos = SDL_WINDOWPOS_CENTERED;

    return false;
}

extern "C" void sdlwDestroyWindow() {
	SdlwContext *sdlw = sdlwContext;
	if (sdlw == NULL) return;
    if (sdlw->window != NULL) {
        delete reinterpret_cast<NMLWinHandler*>(sdlw->window);
        //SDL_DestroyWindow(sdlw->window);
        sdlw->window=NULL;
        sdlw->windowWidth = 0;
        sdlw->windowHeight = 0;
    }
}

extern "C" bool sdlwIsExitRequested()
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return true;
    return sdlw->exitRequested;
}

extern "C" void sdlwRequestExit(bool flag)
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->exitRequested = flag;
}

extern "C" bool sdlwResize(int w, int h) {
	SdlwContext *sdlw = sdlwContext;
	sdlw->windowWidth = w;
	sdlw->windowHeight = h;
    return false;
}

extern "C" void sdlwEnableDefaultEventManagement(bool flag)
{
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->defaultEventManagementEnabled = flag;
}

static void sdlwManageEvent(SdlwContext *sdlw, SDL_Event *event) {
    switch (event->type) {
    default:
        break;

    case SDL_QUIT:
        printf("Exit requested by the system.");
        sdlwRequestExit(true);
        break;

    case SDL_WINDOWEVENT:
        switch (event->window.event) {
        case SDL_WINDOWEVENT_CLOSE:
            printf("Exit requested by the user (by closing the window).");
            sdlwRequestExit(true);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            sdlwResize(event->window.data1, event->window.data2);
            break;
        }
        break;
    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        default:
            break;
        case 27:
            printf("Exit requested by the user (with a key).");
            sdlwRequestExit(true);
            break;
        }
        break;
    }
}

    static SDL_QuitEvent sdlEvent(const GMI::QuitEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count()};
    }

    static SDL_DisplayEvent sdlEvent(const GMI::DisplayEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .display=e.display, .event=e.event, .data1=e.data1};
    }

    static SDL_WindowEvent sdlEvent(const GMI::WindowEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID=e.windowID, .event=static_cast<uint8_t>(e.event), .data1=e.data1 ,
            .data2=e.data2};
    }

    static SDL_MouseMotionEvent sdlEvent(const GMI::MouseMotionEvent  &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID = e.windowID, .which=e.which, 
           .state=e.state,.x =  e.x,.y=e.y,.xrel=e.xrel,.yrel=e.yrel};
    }

    static SDL_MouseButtonEvent sdlEvent(const GMI::MouseButtonEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID=e.windowID, .which=e.which, .state=e.state, 
           .clicks=e.clicks, .x=e.x, .y=e.y};
    }

    static SDL_MouseWheelEvent sdlEvent(const GMI::MouseWheelEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID=e.windowID, .which=e.which, .x=e.x, 
           .y=e.y, .direction=e.direction, .preciseX=e.preciseX, .preciseY=e.preciseY};
    }

    static SDL_Keysym sdlEvent(const GMI::Keysym &e)
    {
        return {.scancode=static_cast<SDL_Scancode>(e.scancode),.sym=e.sym,.mod=e.mod,.unused=e.unused};
    }

    static SDL_KeyboardEvent sdlEvent(const GMI::KeyboardEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID=e.windowID, .state=e.state, 
            .repeat=e.repeat, .keysym=sdlEvent(e.keysym)};
    }

    static SDL_TouchFingerEvent sdlEvent(const GMI::TouchFingerEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .touchId=e.touchId, .fingerId=e.fingerId, .x=e.x, 
           .y=e.y, .dx=e.dx, .dy=e.dy, .pressure=e.pressure, .windowID=e.windowID};
    }

    static SDL_MultiGestureEvent sdlEvent(const GMI::MultiGestureEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .touchId=e.touchId, .dTheta=e.dTheta, .dDist=e.dDist, 
           .x=e.x, .y=e.y, .numFingers=e.numFingers};
    }

    static SDL_UserEvent sdlEvent(const GMI::UserEvent &e)
    {
        return {.type = static_cast<uint32_t>(e.type), .timestamp = e.timestamp.count(), .windowID=e.windowID, .code=e.code, .data1=e.data1,
            .data2=e.data2};
    }

    static std::tuple<SDL_Event, bool> sdlEvent(const GMI::Event &event)
    {
        SDL_Event sdl_Event = {};
        
        bool b = false;
        switch (event.type)
        {
        case GMI::EventType::KEYDOWN:
        case GMI::EventType::KEYUP:
            b = true;
            sdl_Event.key = sdlEvent(event.key);
            break;
        case GMI::EventType::MOUSEWHEEL:
            b = true;
            sdl_Event.wheel = sdlEvent(event.wheel);
            break;
        case GMI::EventType::MOUSEBUTTONDOWN:
        case GMI::EventType::MOUSEBUTTONUP:
            b = true;
            sdl_Event.button = sdlEvent(event.button);
            break;
        case GMI::EventType::MOUSEMOTION:
            b = true;
            sdl_Event.motion = sdlEvent(event.motion);
            break;
        case GMI::EventType::WINDOWEVENT:
            b = true;
            sdl_Event.window = sdlEvent(event.window);
            break;
        case GMI::EventType::DISPLAYEVENT:
            b = true;
            sdl_Event.display = sdlEvent(event.display);
            break;
        case GMI::EventType::FINGERDOWN:
        case GMI::EventType::FINGERUP:
        case GMI::EventType::FINGERMOTION:
            b = true;
            sdl_Event.tfinger = sdlEvent(event.tfinger);
            break;
        case GMI::EventType::MULTIGESTURE:
            b = true;
            sdl_Event.mgesture = sdlEvent(event.mgesture);
            break;
        case GMI::EventType::QUIT:
            b = true;
            sdl_Event.quit = sdlEvent(event.quit);
            break;
        case GMI::EventType::USEREVENT:
            b = true;
            sdl_Event.user = sdlEvent(event.user);
            break;
        }

        return std::make_tuple(sdl_Event, b);
    }


void sdlwCheckEvents() {
	SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    
    std::mutex* pMutex = reinterpret_cast<std::mutex*>(sdlw->pEventMutex);
    std::queue<GMI::Event>* pEventQueue = reinterpret_cast<std::queue<GMI::Event>*>(sdlw->pEventQueue);
	std::tuple<SDL_Event, bool> event;
    GMI::Event gevent;
	while (true) {
        bool eventManaged = false;
		SdlProcessEventFunction processEvent = sdlw->processEvent;
        {
            const std::lock_guard lock(*pMutex);
            if(!pEventQueue->empty()){
                gevent= pEventQueue->front();
                pEventQueue->pop();
            }else{
                break;
            }
        }
        event = sdlEvent(gevent);
		if (processEvent != NULL && std::get<1>(event)){
			eventManaged = processEvent(&std::get<0>(event));
        }

        if (!eventManaged && sdlw->defaultEventManagementEnabled){
            sdlwManageEvent(sdlw, &std::get<0>(event));
        }
            
	}
}

#ifdef SAILFISHOS
SDL_DisplayOrientation sdlwCurrentOrientation() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SDL_ORIENTATION_UNKNOWN;
    return sdlw->orientation;
}

SDL_DisplayOrientation sdlwGetRealOrientation() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SDL_ORIENTATION_UNKNOWN;
    return sdlw->real_orientation;
}

void sdlwSetOrientation(SDL_DisplayOrientation orientation) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->orientation = orientation;

    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdlwContext->window, &wmInfo)) {
        printf("Cannot get the window handle.\n");
        // goto on_error;
        switch (sdlw->orientation) {
            case SDL_ORIENTATION_LANDSCAPE:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
                break;
            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-landscape");
                break;
            case SDL_ORIENTATION_PORTRAIT:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"portrait");
                break;
            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-portrait");
                break;
            default:
            case SDL_ORIENTATION_UNKNOWN:
                SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
                // printf("SDL_DisplayOrientation is SDL_ORIENTATION_UNKNOWN\n");
                break;
        }
    }
    // nativeDisplay = wmInfo.info.wl.display;
    // wl_surface *sdl_wl_surface = wmInfo.info.wl.surface;

    // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
    switch (sdlw->orientation) {
        case SDL_ORIENTATION_LANDSCAPE:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_270);
            break;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_90);
            break;
        case SDL_ORIENTATION_PORTRAIT:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"portrait");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_NORMAL);
            break;
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"inverted-portrait");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_180);
            break;
        default:
        case SDL_ORIENTATION_UNKNOWN:
            // SDL_SetHint(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION,"landscape");
            wl_surface_set_buffer_transform(wmInfo.info.wl.surface, WL_OUTPUT_TRANSFORM_90);
            // printf("SDL_DisplayOrientation is SDL_ORIENTATION_UNKNOWN\n");
            break;
    }
}

void sdlwSetRealOrientation(SDL_DisplayOrientation orientation) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    sdlw->real_orientation = orientation;
}
#endif

#ifdef SAILFISH_FBO
float sdlwGetFboScale() {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return SAILFISH_FBO_DEFAULT_SCALE;
    return sdlw->fbo_scale;
}

void sdlwSetFboScale(float scale) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return;
    if( scale <= 0.0f )
    // TODO should calculate min width 320
        scale = 0.2f;
    else if( scale > 1.0f )
        scale = 1.0f;
    sdlw->fbo_scale = scale;
}

#endif

extern "C" void sdlwGetWindowSize(int *w, int *h) {
    SdlwContext *sdlw = sdlwContext;
    if (sdlw == NULL) return; 
#ifdef SAILFISHOS
    switch (sdlw->orientation) {
        case SDL_ORIENTATION_PORTRAIT:
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            *h = sdlw->windowWidth;
            *w = sdlw->windowHeight;
            break;
        case SDL_ORIENTATION_LANDSCAPE:
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
        case SDL_ORIENTATION_UNKNOWN:
        default:
            *w = sdlw->windowWidth;
            *h = sdlw->windowHeight;
            break;
    }
#else
    *w = sdlw->windowWidth;
    *h = sdlw->windowHeight;
#endif
}
