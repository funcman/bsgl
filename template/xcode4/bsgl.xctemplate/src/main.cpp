#include <bsgl.h>

BSGL* bsgl = 0;

bool LogicFunc() {
    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(RGBA(0xFF, 0xFF, 0xFF, 0xFF));
    bsgl->Gfx_EndScene();
    return false;
}

void bsgl_begin() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateInt(BSGL_NUMOFLFPS, 5);
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateInt(BSGL_NUMOFRFPS, 25);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "bsgl");

    bsgl->System_Initiate();
}

void bsgl_end() {
    bsgl->System_Shutdown();
    bsgl->Release();
}
