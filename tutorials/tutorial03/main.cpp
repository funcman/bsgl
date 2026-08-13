/*
** BSGL Tutorial 03 - Using helper classes
** (adapted from hge_tut03 - Using helper classes)
**
** bsglSprite wraps a bsglQuad with a texture region, hotspot,
** color tinting and several render modes.
** bsglAnimation extends bsglSprite with frame-by-frame playback
** from a texture strip.
**
** Move the orb with the arrow keys (same movement as tutorial 02).
** Press SPACE to start/stop the star animation.
** Press ESC to quit.
*/

#include "bsgl.h"
#include "bsglsprite.h"
#include "bsglanim.h"

static BSGL* bsgl = nullptr;

// Pointers to the BSGL helper objects we will use
static bsglSprite*    spr  = nullptr;
static bsglAnimation* anim = nullptr;

// Handle for the texture
static HTEXTURE tex = 0;

// Some "gameplay" variables
static float x = 100.0f, y = 100.0f;
static float dx = 0.0f, dy = 0.0f;
static float rot = 0.0f;

static const float speed    = 90;
static const float friction = 0.98f;

static bool KeyHeld(int key) {
    return bsgl->Control_IsDown(key) || bsgl->Control_IsPassing(key);
}

bool LogicFunc() {
    float dt = bsgl->Timer_GetDelta();

    bsgl->Control_GetState();

    // Process keys
    if (bsgl->Control_IsDown(INP_ESC)) {
        return true;
    }
    if (KeyHeld(INP_LEFT)) dx -= speed*dt;
    if (KeyHeld(INP_RIGHT)) dx += speed*dt;
    if (KeyHeld(INP_UP)) dy -= speed*dt;
    if (KeyHeld(INP_DOWN)) dy += speed*dt;

    // SPACE toggles the animation playback
    if (bsgl->Control_IsDown(INP_SPACE)) {
        if (anim->IsPlaying()) {
            anim->Stop();
        } else {
            anim->Play();
        }
    }

    // Do some movement calculations and collision detection
    dx *= friction;
    dy *= friction;
    x += dx;
    y += dy;
    if (x > 784) { x = 784 - (x-784); dx = -dx; }
    if (x < 16) { x = 16 + 16 - x;   dx = -dx; }
    if (y > 584) { y = 584 - (y-584); dy = -dy; }
    if (y < 16) { y = 16 + 16 - y;   dy = -dy; }

    // Spin the orb slowly
    rot += dt;

    // Advance the animation to the next frame when due
    anim->Update(dt);

    return false;
}

bool RenderFunc() {
    bsgl->Gfx_BeginScene();
    bsgl->Gfx_Clear(0);

    // Render the animation at a fixed spot
    anim->Render(400, 300);

    // Render the sprite at its position, rotated around its hotspot
    spr->RenderEx(x, y, rot);

    bsgl->Gfx_EndScene();

    return false;
}

void bsgl_main() {
    bsgl = bsglCreate(BSGL_VERSION);

    bsgl->System_SetStateString(BSGL_LOGFILE, "tutorial03.log");
    bsgl->System_SetStateFunc(BSGL_LOGICFUNC, LogicFunc);
    bsgl->System_SetStateFunc(BSGL_RENDERFUNC, RenderFunc);
    bsgl->System_SetStateString(BSGL_TITLE, "BSGL Tutorial 03 - Using helper classes");
    bsgl->System_SetStateInt(BSGL_NUMOFLFPS, 100);
    bsgl->System_SetStateBool(BSGL_WINDOWED, true);
    bsgl->System_SetStateInt(BSGL_SCREENWIDTH, 800);
    bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, 600);

    if (bsgl->System_Initiate()) {
        // Load both textures; anim.bmp holds six 48x48 frames side by side
        tex = bsgl->Texture_Load("sprite.bmp");
        HTEXTURE anim_tex = bsgl->Texture_Load("anim.bmp");
        if (!tex || !anim_tex) {
            bsgl->System_Log("Error: can't load sprite.bmp or anim.bmp");
            bsgl->System_Shutdown();
            bsgl->Release();
            return;
        }

        // Create and set up a sprite:
        // the whole 64x64 texture, tinted orange, hotspot at the center
        // so that RenderEx() rotates it in place
        spr = new bsglSprite(tex, 0, 0, 64, 64);
        spr->SetColor(RGBA(0xFF, 0xA0, 0x00, 0xFF));
        spr->SetHotSpot(32, 32);

        // Create and set up an animation:
        // 6 frames at 10 FPS from the 288x48 strip, additive glow
        anim = new bsglAnimation(anim_tex, 6, 10.0f, 0, 0, 48, 48);
        anim->SetHotSpot(24, 24);
        anim->SetBlendMode(BLEND_COLORMUL | BLEND_ALPHAADD | BLEND_NOZWRITE);
        anim->SetMode(BSGLANIM_FWD | BSGLANIM_LOOP);
        anim->Play();

        // Let's rock now!
        bsgl->System_Start();

        // Delete created objects and free loaded resources
        delete anim;
        delete spr;
        bsgl->Texture_Free(anim_tex);
        bsgl->Texture_Free(tex);

        bsgl->System_Shutdown();
    } else {
        bsgl->System_Log("Error: %s", bsgl->System_GetErrorMessage());
    }

    bsgl->Release();
}
