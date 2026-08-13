/*
** BSGL Tutorial 07 - Thousand of Mushrooms
** (adapted from hge_tut07 - Thousand of Mushrooms)
**
** A sprite batching stress test: up to 2000 sprites flying around,
** bouncing off the screen edges, scaling and rotating independently.
**
**   UP / DOWN - adjust the number of mushrooms by 100
**   SPACE     - cycle through 5 blending modes
**   ESC       - quit
**
** The mushroom count, blend mode and FPS are displayed in the top left
** corner with a bsglFont text texture (see tutorial 04).
**
** Required resource: "mushroom.bmp" - an uncompressed 32-bit BMP
** holding the mushroom in its top left 64x64 pixels. See
** tutorials/README.md for how to provide it.
*/

#include "bsgl.h"
#include "bsglsprite.h"
#include "bsglfont.h"
#include <stdio.h>
#include <string.h>
#include <vector>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

#define MIN_OBJECTS 100
#define MAX_OBJECTS 10000

struct sprObject {
    float x, y;
    float dx, dy;
    float scale, rot;
    float dscale, drot;
    DWORD color;
};

static std::vector<sprObject> pObjects;
static int nObjects;
static int nBlend;

// Pointer to the BSGL interface (helper classes require this to work)
static BSGL* bsgl = nullptr;

// Resource handles
static HTEXTURE    tex = 0;
static bsglSprite* spr = nullptr;
static bsglQuad    bgquad;

// Text display
static HTEXTURE    text_tex = 0;
static bsglSprite* text_spr = nullptr;
static bsglFont*   font     = nullptr;
static float       text_dt  = 1.0f; // force an immediate redraw

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

// Set up blending mode for the scene
static void SetBlend(int blend) {
    static int sprBlend[5] = {
        BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE,
        BLEND_COLORADD | BLEND_ALPHABLEND | BLEND_NOZWRITE,
        BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE,
        BLEND_COLORMUL | BLEND_ALPHAADD  | BLEND_NOZWRITE,
        BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE,
    };

    static DWORD fntColor[5] = {
        RGBA(0xFF, 0xFF, 0xFF, 0xFF), RGBA(0x00, 0x00, 0x00, 0xFF),
        RGBA(0xFF, 0xFF, 0xFF, 0xFF), RGBA(0x00, 0x00, 0x00, 0xFF),
        RGBA(0xFF, 0xFF, 0xFF, 0xFF),
    };

    static DWORD sprColors[5][5] = {
        {RGBA(0xFF, 0xFF, 0xFF, 0xFF), RGBA(0xFF, 0xE0, 0x80, 0xFF), RGBA(0x80, 0xA0, 0xFF, 0xFF), RGBA(0xA0, 0xFF, 0x80, 0xFF), RGBA(0xFF, 0x80, 0xA0, 0xFF)},
        {RGBA(0x00, 0x00, 0x00, 0xFF), RGBA(0x30, 0x30, 0x00, 0xFF), RGBA(0x00, 0x00, 0x60, 0xFF), RGBA(0x00, 0x60, 0x00, 0xFF), RGBA(0x60, 0x00, 0x00, 0xFF)},
        {RGBA(0xFF, 0xFF, 0xFF, 0x80), RGBA(0xFF, 0xE0, 0x80, 0x80), RGBA(0x80, 0xA0, 0xFF, 0x80), RGBA(0xA0, 0xFF, 0x80, 0x80), RGBA(0xFF, 0x80, 0xA0, 0x80)},
        {RGBA(0xFF, 0xFF, 0xFF, 0x80), RGBA(0xFF, 0xE0, 0x80, 0x80), RGBA(0x80, 0xA0, 0xFF, 0x80), RGBA(0xA0, 0xFF, 0x80, 0x80), RGBA(0xFF, 0x80, 0xA0, 0x80)},
        {RGBA(0x20, 0x20, 0x20, 0x40), RGBA(0x30, 0x20, 0x10, 0x40), RGBA(0x10, 0x20, 0x30, 0x40), RGBA(0x20, 0x30, 0x10, 0x40), RGBA(0x10, 0x20, 0x30, 0x40)},
    };

    if (blend > 4) blend = 0;
    nBlend = blend;

    spr->SetBlendMode(sprBlend[blend]);
    if (text_spr) {
        text_spr->SetColor(fntColor[blend]);
    }
    for( int i=0; i<MAX_OBJECTS; ++i ) {
        pObjects[i].color = sprColors[blend][bsgl->Random_Int(0, 4)];
    }

    text_dt = 1.0f; // redraw the text with the new colors
}

// Redraw the info text (mushroom count, blend mode, FPS)
static void UpdateText() {
    if (!font) {
        return;
    }

    int w = bsgl->Texture_GetWidth(text_tex);
    int h = bsgl->Texture_GetHeight(text_tex);
    DWORD* pixels = bsgl->Texture_CreateData(w, h);
    memset(pixels, 0, w*h*sizeof(DWORD));
    bsgl->Texture_Update(text_tex, pixels, 0, 0, w, h);
    bsgl->Texture_FreeData(pixels);

    char buf[96];
    sprintf(buf, "UP/DOWN to adjust mushrooms: %d", nObjects);
    font->BeginDrawTexture(text_tex, 4, 20, 24);
    for( char const* p = buf; *p; ++p ) font->DrawGlyph((wchar_t)*p);
    font->EndDrawTexture();

    sprintf(buf, "SPACE for blend mode: %d   FPS: %d", nBlend, bsgl->Timer_GetFPS());
    font->BeginDrawTexture(text_tex, 4, 44, 24);
    for( char const* p = buf; *p; ++p ) font->DrawGlyph((wchar_t)*p);
    font->EndDrawTexture();
}

bool LogicFunc() {
    float dt = bsgl->Timer_GetDelta();

    bsgl->Control_GetState();

    // Process keys
    if (bsgl->Control_IsDown(INP_ESC)) {
        return true;
    }
    if (bsgl->Control_IsDown(INP_UP)) {
        if (nObjects < MAX_OBJECTS) nObjects += 100;
        text_dt = 1.0f;
    }
    if (bsgl->Control_IsDown(INP_DOWN)) {
        if (nObjects > MIN_OBJECTS) nObjects -= 100;
        text_dt = 1.0f;
    }
    if (bsgl->Control_IsDown(INP_SPACE)) {
        SetBlend(++nBlend);
    }

    // Update the scene
    for( int i=0; i<nObjects; ++i ) {
        pObjects[i].x += pObjects[i].dx*dt;
        if (pObjects[i].x > SCREEN_WIDTH || pObjects[i].x < 0) {
            pObjects[i].dx = -pObjects[i].dx;
        }
        pObjects[i].y += pObjects[i].dy*dt;
        if (pObjects[i].y > SCREEN_HEIGHT || pObjects[i].y < 0) {
            pObjects[i].dy = -pObjects[i].dy;
        }
        pObjects[i].scale += pObjects[i].dscale*dt;
        if (pObjects[i].scale > 2 || pObjects[i].scale < 0.5f) {
            pObjects[i].dscale = -pObjects[i].dscale;
        }
        pObjects[i].rot += pObjects[i].drot*dt;
    }

    // Redraw the info text a few times a second (for the FPS readout)
    text_dt += dt;
    if (text_dt >= 0.25f) {
        text_dt = 0.0f;
        UpdateText();
    }

    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();

    // Background: an untextured fullscreen quad with per-vertex colors
    bsgl->Gfx_RenderQuad(&bgquad);

    // Render the mushrooms
    for( int i=0; i<nObjects; ++i ) {
        spr->SetColor(pObjects[i].color);
        spr->RenderEx(pObjects[i].x, pObjects[i].y,
                      pObjects[i].rot, pObjects[i].scale);
    }

    if (text_spr) {
        text_spr->Render(7, 7);
    }

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial07.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 07 - Thousand of Mushrooms");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, SCREEN_WIDTH);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, SCREEN_HEIGHT);

    if (bsgl->System_Initiate()) {
        // Load the mushroom texture (see the header comment about the format)
        tex = bsgl->Texture_Load("mushroom.bmp");
        if (!tex) {
            bsgl->System_Log("Error: can't load mushroom.bmp (see tutorials/README.md)");
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }

        // Create the mushroom sprite: top left 64x64 of the texture
        spr = new bsglSprite(tex, 0, 0, 64, 64);
        spr->SetHotSpot(32, 32);

        // Set up the background quad: black at the top, dark blue
        // at the bottom (the HGE original uses a textured background
        // with the same per-vertex color trick)
        bgquad.tex   = 0;
        bgquad.blend = BLEND_COLORADD | BLEND_ALPHABLEND | BLEND_NOZWRITE;
        bgquad.v[0].x = 0;             bgquad.v[0].y = 0;
        bgquad.v[1].x = SCREEN_WIDTH;  bgquad.v[1].y = 0;
        bgquad.v[2].x = SCREEN_WIDTH;  bgquad.v[2].y = SCREEN_HEIGHT;
        bgquad.v[3].x = 0;             bgquad.v[3].y = SCREEN_HEIGHT;
        bgquad.v[0].color = RGBA(0x00, 0x00, 0x00, 0xFF);
        bgquad.v[1].color = RGBA(0x00, 0x00, 0x00, 0xFF);
        bgquad.v[2].color = RGBA(0x00, 0x00, 0x40, 0xFF);
        bgquad.v[3].color = RGBA(0x00, 0x00, 0x40, 0xFF);
        for( int i=0; i<4; ++i ) {
            bgquad.v[i].z = 0.0f;
        }

        // Set up the info text display
        if (FontFileExists()) {
            bsgl->System_Log("Using font file: %s", FONT_FILE);
            font = new bsglFont(FONT_FILE, 14);
            text_tex = bsgl->Texture_Create(320, 64);
            text_spr = new bsglSprite(text_tex, 0, 0, 320, 64);
        } else {
            bsgl->System_Log("Warning: %s not found, no on-screen text.", FONT_FILE);
        }

        // Initialize the objects list
        pObjects.resize(MAX_OBJECTS);
        nObjects = 1000;

        for( int i=0; i<MAX_OBJECTS; ++i ) {
            pObjects[i].x      = bsgl->Random_Float(0, SCREEN_WIDTH);
            pObjects[i].y      = bsgl->Random_Float(0, SCREEN_HEIGHT);
            pObjects[i].dx     = bsgl->Random_Float(-200, 200);
            pObjects[i].dy     = bsgl->Random_Float(-200, 200);
            pObjects[i].scale  = bsgl->Random_Float(0.5f, 2.0f);
            pObjects[i].dscale = bsgl->Random_Float(-1.0f, 1.0f);
            pObjects[i].rot    = bsgl->Random_Float(0, M_PI*2);
            pObjects[i].drot   = bsgl->Random_Float(-1.0f, 1.0f);
        }

        SetBlend(0);

        // Let's rock now!
        bsgl->System_Start();

        // Delete created objects and free loaded resources
        delete font;
        delete text_spr;
        if (text_tex) {
            bsgl->Texture_Free(text_tex);
        }
        delete spr;
        bsgl->Texture_Free(tex);

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
