/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** Core system functions (SDL3 frontend)
*/

#include "bsgl_impl.h"

#include <SDL3/SDL_main.h>

#ifdef _WIN32
#include <direct.h>  // _chdir
#else
#include <unistd.h>  // chdir
#endif

#define _KEY_BUF_SIZE 256

extern void _InitOGL();
extern void _Resize(int, int);

extern void bsgl_main();

bool isRunning = false;

int         nRef = 0;
BSGL_Impl*   pBSGL = 0;
static bool  bInit = false;

BSGL* CALL bsglCreate(int ver) {
    if( BSGL_VERSION == ver ) {
        return (BSGL*)BSGL_Impl::_Interface_Get();
    } else {
        return 0;
    }
}

BSGL_Impl* BSGL_Impl::_Interface_Get() {
    if( 0 == pBSGL ) {
        pBSGL = new BSGL_Impl();
    }
    ++nRef;
    return pBSGL;
}

void CALL BSGL_Impl::Release() {
    --nRef;
    if( 0 == nRef) {
        delete pBSGL;
        pBSGL = 0;
    }
}

bool CALL BSGL_Impl::System_Initiate() {
    System_Log("BSGL Started...\n");
    System_Log("BSGL version: %X.%X", BSGL_VERSION>>8, BSGL_VERSION&0xFF);
    System_Log("Application: %s\n", szTitle);

    _LoadConfig("config.xml");

    Uint32 flags = SDL_WINDOW_OPENGL;
    if( !bWindowed ) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    window = SDL_CreateWindow(szTitle[0] ? szTitle : "BSGL GAME",
                              nScreenWidth, nScreenHeight, flags);
    if( 0 == window ) {
        _PostError("Can't create the window: %s", SDL_GetError());
        System_Shutdown();
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    gl_context = SDL_GL_CreateContext(window);
    if( 0 == gl_context ) {
        _PostError("Can't create the OpenGL context: %s", SDL_GetError());
        System_Shutdown();
        return false;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    _InitOGL();
    _Resize(nScreenWidth, nScreenHeight);

    if( !_GfxInit() ) {
        _PostError("Can't initialize OpenGL.");
        System_Shutdown();
        return false;
    }

    fTime = 0.0f;

    bInit = true;

    return true;
}

void CALL BSGL_Impl::System_Shutdown() {
    System_Log("\nFinishing...");
    _GfxDone();

    if( gl_context ) {
        SDL_GL_DestroyContext(gl_context);
        gl_context = 0;
    }
    if( window ) {
        SDL_DestroyWindow(window);
        window = 0;
    }

    System_Log("The End.");
}

static void _PumpEvents() {
    SDL_Event event;
    while( SDL_PollEvent(&event) ) {
        switch( event.type ) {
            case SDL_EVENT_QUIT:
                isRunning = false;
                break;
            default:
                break;
        }
    }
}

bool CALL BSGL_Impl::System_Start() {
    float freq = (float)SDL_GetPerformanceFrequency();
    float time = (float)SDL_GetPerformanceCounter() / freq;
    float old_time = time;
    float old_logic_time = time;
    float old_render_time = time;
    float time4fps = time;
    float logic_dt = 0.0f;
    float render_dt = 0.0f;
    unsigned int fps = 0;

    while( isRunning ) {
        _PumpEvents();

        time = (float)SDL_GetPerformanceCounter() / freq;
        float _t = time - old_time;
        old_time = time;
        fTime += _t;
        if( !bActive ) {
            old_logic_time += _t;
            old_render_time += _t;
            SDL_DelayNS(1000);
            continue;
        }

        if( ( 0 == nLFPS )
         || ( (time-old_logic_time) >= (1.0f/nLFPS) ) ) {
            fDeltaTime = logic_dt;
            if( fpLogicFunc != 0 ) {
                if( fpLogicFunc() ) {
                    break;
                }
            }
            //fLogicDeltaTime
            logic_dt = ( time - old_logic_time );
            if( nLFPS != 0 ) {
                old_logic_time += (1.0f/nLFPS);
            }else {
                old_logic_time = time;
            }
        }

        if( ( 0 == nRFPS )
         || ( (time-old_render_time) >= (1.0f/nRFPS) ) ) {
            fDeltaTime = render_dt;
            if( fpRenderFunc != 0 ) {
                if( fpRenderFunc() ) {
                    break;
                }
                ++fps;
            }
            SDL_GL_SwapWindow(window);

            //fRenderDeltaTime
            render_dt = ( time - old_render_time );
            if( nRFPS != 0 ) {
                old_render_time += (1.0f/nRFPS);
            }else {
                old_render_time = time;
            }
        }

        if( time-time4fps >= 1.0f ) {
            //System_Log("fps: %u\ntime: %f", fps, Timer_GetTime());
            nFPS = fps;
            fps = 0;
            time4fps += 1.0f;
        }

        SDL_DelayNS(1000);
    }
    return true;
}

void CALL BSGL_Impl::System_SetStateBool(bsglBoolState state, bool value) {
    switch( state ) {
        case BSGL_WINDOWED:
            bWindowed = value;
            break;
        default:
            break;
    }
}

void CALL BSGL_Impl::System_SetStateFunc(bsglFuncState state, bsglCallback value) {
    switch( state ) {
        case BSGL_LOGICFUNC:
            fpLogicFunc = value;
            break;
        case BSGL_RENDERFUNC:
            fpRenderFunc = value;
            break;
        //case BSGL_INPUTFUNC:
        //    procInputFunc = value;
        //    break;
        default:
            break;
    }
}

void CALL BSGL_Impl::System_SetStateInt(bsglIntState state, int value) {
    switch( state ) {
        case BSGL_SCREENWIDTH:
            nScreenWidth = value;
            break;
        case BSGL_SCREENHEIGHT:
            nScreenHeight = value;
            break;
        case BSGL_NUMOFLFPS:
            nLFPS = value;
            break;
        case BSGL_NUMOFRFPS:
            nRFPS = value;
            break;
        case BSGL_POLYMODE:
            nPolyMode = value;
            break;
        default:
            break;
    }
}

int CALL BSGL_Impl::System_GetStateInt(bsglIntState state) {
    switch( state ) {
        case BSGL_SCREENWIDTH:
            return nScreenWidth;
            break;
        case BSGL_SCREENHEIGHT:
            return nScreenHeight;
            break;
        case BSGL_NUMOFLFPS:
            return nLFPS;
            break;
        case BSGL_NUMOFRFPS:
            return nRFPS;
            break;
        case BSGL_POLYMODE:
            return nPolyMode;
            break;
        default:
            break;
    }
    return 0;
}

void CALL BSGL_Impl::System_SetStateString(bsglStringState state, const char* value) {
    switch( state ) {
        case BSGL_TITLE:
            if( value != 0 ) {
                strcpy(szTitle, value);
            }else {
                szTitle[0] = (char)0;
            }
            break;
        case BSGL_CFGFILE:
            if( value != 0 ) {
                strcpy(szCfgFile, value);
            }else {
                szCfgFile[0] = (char)0;
            }
            break;
        case BSGL_LOGFILE:
            if( value != 0 ) {
                FILE* file = fopen(value, "a");
                if (file) {
                    fclose(file);
                    strcpy(szLogFile, value);
                } else {
                    szLogFile[0] = (char)0;
                }
            }else {
                szLogFile[0] = (char)0;
            }
            break;
        default:
            break;
    }
}

char* CALL BSGL_Impl::System_GetErrorMessage() {
    return szError;
}

void CALL BSGL_Impl::System_Log(const char *szFormat, ...) {
    va_list vl;

    if(!szLogFile[0]) {
        return;
    }

    FILE* file = fopen(szLogFile, "a");
    if (!file) {
        return;
    }

    char buffer[1024];

    va_start(vl, szFormat);
    int n = vsnprintf(buffer, 1023, szFormat, vl);
    buffer[n] = 0;
    fwrite(buffer, 1, n, file);
    va_end(vl);
    va_start(vl, szFormat);
    vfprintf(stdout, szFormat, vl);
    va_end(vl);

    fwrite("\n", 1, 1, file);
    fprintf(stdout, "\n");

    fclose(file);
}

BSGL_Impl::BSGL_Impl() {
    bWindowed = false;
    nScreenWidth = 800;
    nScreenHeight = 600;
    nScreenBPP = 32;
    szTitle[0] = (char)0;
    szError[0] = (char)0;
    szLogFile[0] = (char)0;
    bActive = true;
    nLFPS = 60;
    nRFPS = 0;
    fpLogicFunc     = 0;
    fpRenderFunc    = 0;
    fTime = 0.0f;
    fDeltaTime = 0.0f;
    nFPS = 0;
    nPolyMode = 0;
    window = 0;
    gl_context = 0;
    textures = 0;
    indexes = 0;
    VertArray = 0;

    this->_key_buf = new unsigned int[_KEY_BUF_SIZE];
    memset(_key_buf, 0, _KEY_BUF_SIZE*sizeof(unsigned int));
}

void CALL BSGL_Impl::_LoadConfig(char const* filename) {
    if( !filename[0] ) {
        return;
    }
    System_SetStateString(BSGL_CFGFILE, filename);
    bool windowed = Config_GetInt("Screen", "Windowed", bWindowed?1:0)==1 ? true : false;
    System_SetStateBool(BSGL_WINDOWED, windowed);
    int width   = Config_GetInt("Screen", "Width", nScreenWidth);
    int height  = Config_GetInt("Screen", "Height", nScreenHeight);
    System_SetStateInt(BSGL_SCREENWIDTH, width);
    System_SetStateInt(BSGL_SCREENHEIGHT, height);
    int logic_fps   = Config_GetInt("FPS", "Logic", nLFPS);
    int render_fps  = Config_GetInt("FPS", "Render", nRFPS);
    System_SetStateInt(BSGL_NUMOFLFPS, logic_fps);
    System_SetStateInt(BSGL_NUMOFRFPS, render_fps);
    char title[512];
    strcpy(title, Config_GetString("Window", "Title", szTitle[0]?szTitle:"BSGL GAME"));
    System_SetStateString(BSGL_TITLE, title);
}

void BSGL_Impl::_PostError(const char* error, ...) {
    va_list vl;
    char _error[256];

    va_start(vl, error);
    vsprintf(_error, error, vl);
    va_end(vl);

    System_Log("Error: %s", _error);
    strcpy(szError, _error);
}

int main(int argc, char** argv) {
    // Resolve relative paths against the executable's directory, so
    // assets shipped next to the binary are found from any working
    // directory the process happens to be launched from.
    if( const char* base = SDL_GetBasePath() ) {
#ifdef _WIN32
        _chdir(base);
#else
        chdir(base);
#endif
    }

    if( !SDL_Init(SDL_INIT_VIDEO) ) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    isRunning = true;
    bsgl_main();

    SDL_Quit();
    return 0;
}
