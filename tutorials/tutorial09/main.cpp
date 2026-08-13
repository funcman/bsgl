/*
** BSGL Tutorial 09 - DragonBones Skeletal Animation
**
** Plays the official DragonBones "DBMecha" example through the
** bsglDBones class, a wrapper around the DragonBonesCPP runtime
** (3rd/DragonBonesCPP, version 5.6.300).
**
**   LEFT / RIGHT - switch animation
**   ESC          - quit
**
** Required resources (copied next to the executable at build time):
**   DBMecha_ske.json - skeleton/animation data (DragonBones 5.6 export)
**   DBMecha_tex.json - texture atlas data, referencing DBMecha_tex.bmp
**   DBMecha_tex.bmp  - the atlas texture, an uncompressed 32-bit BMP
**
** The current animation name and FPS are displayed in the top left
** corner with a bsglFont text texture (see tutorial 04).
*/

#include "bsgl.h"
#include "bsgldbones.h"
#include "bsglsprite.h"
#include "bsglfont.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

// Pointer to the BSGL interface (helper classes require this to work)
static BSGL* bsgl = nullptr;

// The DragonBones character
static bsglDBones* mecha = nullptr;

// Background: an untextured fullscreen quad with per-vertex colors
static bsglQuad bgquad;

// Text display
static HTEXTURE    text_tex = 0;
static bsglSprite* text_spr = nullptr;
static bsglFont*   font     = nullptr;
static float       text_dt  = 1.0f; // force an immediate redraw

// Index of the current animation, for LEFT/RIGHT switching
static int anim_index = 0;

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

// Switch to the previous/next animation (played in an endless loop)
static void SwitchAnimation(int dir) {
    int count = mecha->GetAnimationCount();
    if (count <= 0) {
        return;
    }

    anim_index = (anim_index + dir + count) % count;
    mecha->Play(mecha->GetAnimationName(anim_index), 0);

    // redraw the info text immediately
    text_dt = 1.0f;
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
    sprintf(buf, "LEFT/RIGHT switch animation   animation: %s",
            mecha->GetCurrentAnimation());
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
    if (bsgl->Control_IsPassing(INP_LEFT)) {
        SwitchAnimation(-1);
    } else if (bsgl->Control_IsPassing(INP_RIGHT)) {
        SwitchAnimation(1);
    }

    // Update the DragonBones animation
    mecha->Update(dt);

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

    mecha->Render();

    if (text_spr) {
        text_spr->Render(7, 7);
    }

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial09.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 09 - DragonBones Skeletal Animation");
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, SCREEN_WIDTH);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, SCREEN_HEIGHT);

    if (bsgl->System_Initiate()) {
        // Load the DBMecha data exported from DragonBones 5.6
        mecha = new bsglDBones();
        if (!mecha->Load("DBMecha_ske.json", "DBMecha_tex.json", "DBMecha_tex.bmp",
                         "DBMecha", "mecha_1406")) {
            bsgl->System_Log("Error: can't load the DBMecha data "
                             "(DBMecha_ske.json / DBMecha_tex.json / DBMecha_tex.bmp)");
            delete mecha;
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }

        // The armature origin is near the feet of the character,
        // place it near the bottom of the window
        mecha->SetPosition(SCREEN_WIDTH/2, SCREEN_HEIGHT-100);
        mecha->SetScale(1.0f, 1.0f);

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

        // Default animation, looped forever
        mecha->Play("idle", 0);

        // Set up the info text display
        if (FontFileExists()) {
            bsgl->System_Log("Using font file: %s", FONT_FILE);
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
        delete mecha; // also frees the atlas texture

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
