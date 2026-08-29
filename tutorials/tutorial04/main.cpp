/*
** BSGL Tutorial 04 - Rendering text with bsglFont
**
** bsglFont rasterizes TrueType fonts with FreeType. With the cached
** DrawText() API every glyph is rasterized exactly once into an
** internal glyph-atlas texture; drawing text then just submits quads
** that sample the atlas via UV. Text color is a per-draw tint, so the
** same glyphs can be drawn in any color.
**
** Typical usage:
**   bsglFont* font = new bsglFont("font.ttf", 40);
**   font->DrawText(x, y, "Hello", RGBA(0xFF, 0xD0, 0x40, 0xFF));
**
** For text that is redrawn every frame with the same layout, a
** bsglTextMesh can be built once and re-rendered instead.
**
** The tutorial uses the bundled DejaVu Sans font (tutorials/res/
** font.ttf), which is copied next to the executable at build time.
**
** Press ESC to quit.
*/

#include "bsgl.h"
#include "bsglfont.h"
#include "bsgltextmesh.h"
#include <stdio.h>
#include <string.h>

static BSGL* bsgl = nullptr;

static bsglTextMesh* title_mesh = nullptr;
static bsglFont*     font_big   = nullptr;
static bsglFont*     font_small = nullptr;

// The bundled DejaVu Sans font (tutorials/res/font.ttf), copied next
// to the executable at build time - see tutorials/CMakeLists.txt.
static const char* const FONT_FILE = "font.ttf";

static bool FontFileExists() {
    FILE* f = fopen(FONT_FILE, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

bool LogicFunc() {
    bsgl->Control_GetState();
    if (bsgl->Control_IsDown(INP_ESC)) {
        return true;
    }
    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(RGBA(0x20, 0x20, 0x40, 0xFF));

    // static title: baked once into a mesh, re-rendered every frame
    if (title_mesh) {
        title_mesh->Render(50, 200, RGBA(0xFF, 0xD0, 0x40, 0xFF));
    }

    // dynamic FPS text: glyphs come from the cache, only the quads
    // are rebuilt - cheap enough to draw every frame
    if (font_small) {
        char buf[64];
        sprintf(buf, "FPS: %d", bsgl->Timer_GetFPS());
        font_small->DrawText(5, 5, buf, RGBA(0x80, 0xFF, 0x80, 0xFF));
    }

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial04.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 04 - Rendering text with bsglFont");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, 800);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, 600);

    if (bsgl->System_Initiate()) {
        if (!FontFileExists()) {
            bsgl->System_Log("Error: %s not found (should have been copied from tutorials/res)", FONT_FILE);
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }
        bsgl->System_Log("Using font file: %s", FONT_FILE);

        font_big   = new bsglFont(FONT_FILE, 40);
        font_small = new bsglFont(FONT_FILE, 16);

        // Static title text, laid out once into a reusable mesh
        title_mesh = new bsglTextMesh();
        font_big->RenderText(title_mesh, "BSGL Tutorial 04");

        bsgl->System_Start();

        delete title_mesh;
        delete font_small;
        delete font_big;

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
