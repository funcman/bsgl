/*
** BSGL Tutorial 04 - Rendering text with bsglFont
**
** bsglFont rasterizes TrueType fonts with FreeType and draws glyphs
** into a texture. The glyphs are composited as opaque white pixels
** with the antialiasing coverage stored in the alpha channel, so the
** text can be tinted to any color with bsglSprite::SetColor().
**
** Typical usage:
**   1. Create a texture with Texture_Create() and clear it to
**      fully transparent pixels.
**   2. BeginDrawTexture() / DrawGlyph()... / EndDrawTexture()
**   3. Render the texture with a bsglSprite.
**
** The tutorial uses the bundled DejaVu Sans font (tutorials/res/
** font.ttf), which is copied next to the executable at build time.
**
** Press ESC to quit.
*/

#include "bsgl.h"
#include "bsglsprite.h"
#include "bsglfont.h"
#include <stdio.h>
#include <string.h>

static BSGL* bsgl = 0;

static HTEXTURE    title_tex = 0;
static HTEXTURE    fps_tex   = 0;
static bsglSprite* title_spr = 0;
static bsglSprite* fps_spr   = 0;
static bsglFont*   font_big   = 0;
static bsglFont*   font_small = 0;

static float fps_timer = 0.0f;

// The bundled DejaVu Sans font (tutorials/res/font.ttf), copied next
// to the executable at build time - see tutorials/CMakeLists.txt.
static const char* const FONT_FILE = "font.ttf";

static bool FontFileExists() {
    FILE* f = fopen(FONT_FILE, "rb");
    if( f ) {
        fclose(f);
        return true;
    }
    return false;
}

// Clear a texture to fully transparent pixels
static void ClearTexture(HTEXTURE tex) {
    int w = bsgl->Texture_GetWidth(tex);
    int h = bsgl->Texture_GetHeight(tex);
    DWORD* pixels = bsgl->Texture_CreateData(w, h);
    memset(pixels, 0, w*h*sizeof(DWORD));
    bsgl->Texture_Update(tex, pixels, 0, 0, w, h);
    bsgl->Texture_FreeData(pixels);
}

// Draw a string into a texture. origin_y is the text baseline.
static void DrawString(bsglFont* font, HTEXTURE tex, int ox, int oy,
                       int line_height, char const* str) {
    font->BeginDrawTexture(tex, ox, oy, line_height);
    for( char const* p = str; *p; ++p ) {
        font->DrawGlyph((wchar_t)*p);
    }
    font->EndDrawTexture();
}

bool LogicFunc() {
    float dt = bsgl->Timer_GetDelta();

    bsgl->Control_GetState();
    if( bsgl->Control_IsDown(INP_ESC) ) {
        return true;
    }

    // Redraw the FPS text twice a second. The texture has to be
    // cleared first, otherwise the new text blends over the old one.
    fps_timer += dt;
    if( fps_timer >= 0.5f ) {
        fps_timer = 0.0f;
        char buf[64];
        sprintf(buf, "FPS: %d", bsgl->Timer_GetFPS());
        ClearTexture(fps_tex);
        DrawString(font_small, fps_tex, 2, 22, 24, buf);
    }

    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(RGBA(0x20, 0x20, 0x40, 0xFF));

    title_spr->Render(50, 200);
    fps_spr->Render(5, 5);

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

    if( bsgl->System_Initiate() ) {
        if( !FontFileExists() ) {
            bsgl->System_Log("Error: %s not found (should have been copied from tutorials/res)", FONT_FILE);
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }
        bsgl->System_Log("Using font file: %s", FONT_FILE);

        font_big   = new bsglFont(FONT_FILE, 40);
        font_small = new bsglFont(FONT_FILE, 16);

        // Static title text, drawn once
        title_tex = bsgl->Texture_Create(512, 64);
        ClearTexture(title_tex);
        DrawString(font_big, title_tex, 4, 48, 48, "BSGL Tutorial 04");
        title_spr = new bsglSprite(title_tex, 0, 0, 512, 64);
        // The glyphs are white; tint them golden
        title_spr->SetColor(RGBA(0xFF, 0xD0, 0x40, 0xFF));

        // Dynamic FPS text, redrawn twice a second in LogicFunc()
        fps_tex = bsgl->Texture_Create(128, 32);
        ClearTexture(fps_tex);
        fps_spr = new bsglSprite(fps_tex, 0, 0, 128, 32);
        fps_spr->SetColor(RGBA(0x80, 0xFF, 0x80, 0xFF));

        bsgl->System_Start();

        delete fps_spr;
        delete title_spr;
        bsgl->Texture_Free(fps_tex);
        bsgl->Texture_Free(title_tex);
        delete font_small;
        delete font_big;

        bsgl->System_Shutdown();
    }else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
