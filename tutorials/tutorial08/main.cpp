/*
** BSGL Tutorial 08 - Spine Skeletal Animation
**
** Plays the official Esoteric Software "spineboy" example through the
** bsglSpine class, a wrapper around the spine-cpp runtime
** (3rd/spine-runtimes, branch 4.2).
**
**   LEFT / RIGHT - walk (with flip)
**   SPACE        - jump
**   ESC          - quit
**
** Required resources (copied next to the executable at build time):
**   spineboy-pro.json - skeleton/animation data (Spine 4.2 export)
**   spineboy.atlas    - texture atlas, referencing spineboy.bmp
**   spineboy.bmp      - atlas page, an uncompressed 32-bit BMP
**                       (converted from the official spineboy.png,
**                       see tutorials/res/spineboy-LICENSE.txt for the
**                       license of the spineboy assets)
**
** The current animation name and FPS are displayed in the top left
** corner with a bsglFont text texture (see tutorial 04).
*/

#include "bsgl.h"
#include "bsglspine.h"
#include "bsglsprite.h"
#include "bsglfont.h"
// full spine-cpp definitions, needed to inspect the events passed to
// OnSpineEvent (the bsglSpine interface itself only forward declares them)
#include <spine/spine.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

// Pointer to the BSGL interface (helper classes require this to work)
static BSGL* bsgl = nullptr;

// The Spine character
static bsglSpine* spineboy = nullptr;

// Background: an untextured fullscreen quad with per-vertex colors
static bsglQuad bgquad;

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

// Spine events are forwarded here (spineboy's walk/run animations
// fire "footstep" events)
static void OnSpineEvent(spine::Event* event, void* userdata) {
    (void)userdata;
    bsgl->System_Log("spine event: %s", event->getData().getName().buffer());
}

static void OnSpineComplete(int trackIndex, int loopCount, void* userdata) {
    (void)loopCount;
    (void)userdata;
    bsgl->System_Log("animation complete on track %d", trackIndex);
}

// Redraw the info text (controls, current animation, FPS)
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
    sprintf(buf, "LEFT/RIGHT walk, SPACE jump   animation: %s",
            spineboy->GetCurrentAnimation(0));
    font->BeginDrawTexture(text_tex, 4, 20, 24);
    for( char const* p = buf; *p; ++p ) font->DrawGlyph((wchar_t)*p);
    font->EndDrawTexture();

    sprintf(buf, "FPS: %d", bsgl->Timer_GetFPS());
    font->BeginDrawTexture(text_tex, 4, 44, 24);
    for( char const* p = buf; *p; ++p ) font->DrawGlyph((wchar_t)*p);
    font->EndDrawTexture();
}

bool LogicFunc() {
    float dt = bsgl->Timer_GetDelta();

    bsgl->Control_GetState();

    if (bsgl->Control_IsDown(INP_ESC)) {
        return true;
    }

    // Keyboard control
    if (bsgl->Control_IsDown(INP_SPACE)) {
        spineboy->SetAnimation(0, "jump", false);
        spineboy->AddAnimation(0, "idle", true, 0.0f);
    } else if (bsgl->Control_IsPassing(INP_LEFT)) {
        spineboy->SetFlip(true, false);
        if (0 != strcmp(spineboy->GetCurrentAnimation(0), "walk")) {
            spineboy->SetAnimation(0, "walk", true);
        }
    } else if (bsgl->Control_IsPassing(INP_RIGHT)) {
        spineboy->SetFlip(false, false);
        if (0 != strcmp(spineboy->GetCurrentAnimation(0), "walk")) {
            spineboy->SetAnimation(0, "walk", true);
        }
    } else if (0 != strcmp(spineboy->GetCurrentAnimation(0), "idle")
           && 0 != strcmp(spineboy->GetCurrentAnimation(0), "jump")) {
        spineboy->SetAnimation(0, "idle", true);
    }

    // Update the Spine animation
    spineboy->Update(dt);

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

    // Background (also clears the previous frame)
    bsgl->Gfx_RenderQuad(&bgquad);

    spineboy->Render();

    if (text_spr) {
        text_spr->Render(7, 7);
    }

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial08.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 08 - Spine Skeletal Animation");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, SCREEN_WIDTH);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, SCREEN_HEIGHT);

    if (bsgl->System_Initiate()) {
        // Load the spineboy data exported from Spine 4.2
        spineboy = new bsglSpine();
        if (!spineboy->Load("spineboy-pro.json", "spineboy.atlas")) {
            bsgl->System_Log("Error: can't load the spineboy data "
                             "(spineboy-pro.json / spineboy.atlas / spineboy.bmp)");
            delete spineboy;
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }

        // Place the feet of the character near the bottom of the window
        spineboy->SetPosition(SCREEN_WIDTH/2, SCREEN_HEIGHT-100);
        spineboy->SetScale(0.5f, 0.5f);

        // Set up the background quad: dark gray at the top, dark blue
        // at the bottom (same per-vertex color trick as tutorial07)
        bgquad.tex   = 0;
        bgquad.blend = BLEND_COLORADD | BLEND_ALPHABLEND | BLEND_NOZWRITE;
        bgquad.v[0].x = 0;             bgquad.v[0].y = 0;
        bgquad.v[1].x = SCREEN_WIDTH;  bgquad.v[1].y = 0;
        bgquad.v[2].x = SCREEN_WIDTH;  bgquad.v[2].y = SCREEN_HEIGHT;
        bgquad.v[3].x = 0;             bgquad.v[3].y = SCREEN_HEIGHT;
        bgquad.v[0].color = RGBA(0x20, 0x20, 0x20, 0xFF);
        bgquad.v[1].color = RGBA(0x20, 0x20, 0x20, 0xFF);
        bgquad.v[2].color = RGBA(0x10, 0x10, 0x40, 0xFF);
        bgquad.v[3].color = RGBA(0x10, 0x10, 0x40, 0xFF);
        for( int i=0; i<4; ++i ) {
            bgquad.v[i].z = 0.0f;
        }

        // Default animation and cross-fades
        spineboy->SetAnimation(0, "idle", true);
        spineboy->SetMix("idle", "walk", 0.2f);
        spineboy->SetMix("walk", "idle", 0.2f);
        spineboy->SetMix("walk", "jump", 0.1f);
        spineboy->SetMix("idle", "jump", 0.1f);

        spineboy->SetEventCallback(OnSpineEvent);
        spineboy->SetCompleteCallback(OnSpineComplete);

        // Set up the info text display
        if (FontFileExists()) {
            font = new bsglFont(FONT_FILE, 14);
            text_tex = bsgl->Texture_Create(560, 64);
            text_spr = new bsglSprite(text_tex, 0, 0, 560, 64);
        } else {
            bsgl->System_Log("Warning: %s not found, no on-screen text.", FONT_FILE);
        }

        // Let's rock now!
        bsgl->System_Start();

        // Delete created objects and free loaded resources
        delete font;
        delete text_spr;
        if (text_tex) {
            bsgl->Texture_Free(text_tex);
        }
        delete spineboy; // also frees the atlas textures

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
