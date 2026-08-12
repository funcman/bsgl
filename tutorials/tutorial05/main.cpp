/*
** BSGL Tutorial 05 - Config file, random numbers, timer and logging
**
** This tutorial shows the small utility subsystems of BSGL:
**
**  - Config_*:  read/write an XML config file (config.xml by default).
**               The window size, title etc. are read from it during
**               System_Initiate(); applications can store their own
**               sections and options in the same file.
**  - Random_*:  a simple portable random number generator.
**  - Timer_*:   elapsed time, frame delta and FPS counter.
**  - System_Log: writes messages both to stdout and the log file.
**
** Every half second a randomly placed, randomly tinted quad spawns.
** The number of times the tutorial has been run is stored in
** config.xml and reported in the log.
**
** Press ESC to quit.
*/

#include "bsgl.h"

static BSGL* bsgl = 0;

#define MAX_QUADS 64

static bsglQuad quads[MAX_QUADS];
static int      nquads    = 0;
static float    spawn_dt  = 0.0f;
static int      run_count = 0;

static void SpawnQuad() {
    if( nquads >= MAX_QUADS ) {
        return;
    }
    bsglQuad* q = &quads[nquads++];
    float cx = bsgl->Random_Float(32, 768);
    float cy = bsgl->Random_Float(32, 568);
    float r  = bsgl->Random_Float(8, 32);
    DWORD col = RGBA(bsgl->Random_Int(64, 255),
                     bsgl->Random_Int(64, 255),
                     bsgl->Random_Int(64, 255),
                     0xC0);

    q->tex   = 0;
    q->blend = BLEND_DEFAULT;
    q->v[0].x = cx-r; q->v[0].y = cy-r;
    q->v[1].x = cx+r; q->v[1].y = cy-r;
    q->v[2].x = cx+r; q->v[2].y = cy+r;
    q->v[3].x = cx-r; q->v[3].y = cy+r;
    for( int i=0; i<4; ++i ) {
        q->v[i].z     = 0.5f;
        q->v[i].color = col;
        q->v[i].tx    = 0.0f;
        q->v[i].ty    = 0.0f;
    }
}

bool LogicFunc() {
    bsgl->Control_GetState();
    if( bsgl->Control_IsDown(INP_ESC) ) {
        return true;
    }

    // Spawn a quad twice a second, using the logic frame delta
    spawn_dt += bsgl->Timer_GetDelta();
    if( spawn_dt >= 0.5f ) {
        spawn_dt = 0.0f;
        SpawnQuad();
    }

    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(RGBA(0x10, 0x10, 0x18, 0xFF));

    for( int i=0; i<nquads; ++i ) {
        bsgl->Gfx_RenderQuad(&quads[i]);
    }

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial05.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 05 - Config, random, timer and logging");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, 800);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, 600);

    if( bsgl->System_Initiate() ) {
        // Seed the RNG. Passing 0 seeds it from the system tick count.
        bsgl->Random_Seed(0);

        // The config file (config.xml in the working directory) was
        // loaded during System_Initiate(). Read and bump the run count.
        run_count = bsgl->Config_GetInt("Tutorial05", "RunCount", 0) + 1;
        bsgl->System_Log("This tutorial has been run %d time(s).", run_count);
        bsgl->Config_SetInt("Tutorial05", "RunCount", run_count);

        // Demonstrate the other config accessors
        bsgl->Config_SetString("Tutorial05", "Greeting", "hello bsgl");
        bsgl->System_Log("Greeting read back: %s",
                         bsgl->Config_GetString("Tutorial05", "Greeting", ""));
        bsgl->Config_SetFloat("Tutorial05", "SomeFactor", 1.5f);
        bsgl->System_Log("SomeFactor read back: %.2f",
                         bsgl->Config_GetFloat("Tutorial05", "SomeFactor", 0.0f));

        // Timer_GetTime() returns seconds since System_Initiate()
        bsgl->System_Log("Uptime at start: %.3fs", bsgl->Timer_GetTime());

        bsgl->System_Start();

        bsgl->System_Log("Uptime at exit: %.3fs, quads spawned: %d",
                         bsgl->Timer_GetTime(), nquads);

        bsgl->System_Shutdown();
    }else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
