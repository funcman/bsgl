/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglWren: hosts a Wren VM and binds the BSGL API to Wren.
**
** The embedded library (src/advance/bsgl.wren, module "bsgl") holds
** the foreign declarations; this file implements them. Run() drives
** the whole lifecycle:
**
**   1. interpret the script (top-level: set system states, define
**      classes, create the global `var main`)
**   2. System_Initiate()
**   3. call main.init()          (create resources here)
**   4. System_Start()            (calls main.update(dt) / main.render())
**   5. free the VM (runs finalizers, releasing sprites/textures),
**      then System_Shutdown()
**
** Wren forbids calling into the VM from inside a foreign method
** (wren_vm.c: "Can not call from a foreign method."), so callbacks
** that originate from C++ code running inside a foreign call - the
** bsglWidget virtuals during mouseAt()/render() and the Spine event
** listeners during update() - are queued and dispatched from the
** frame bridge, after the update()/render() call has returned.
*/

#include "bsgl.h"
#include "bsglsprite.h"
#include "bsglanim.h"
#include "bsglfont.h"
#include "bsglspine.h"
#include "bsgldbones.h"
#include "bsglwidget.h"
#include "bsglwren.h"

// wren.h has no extern "C" guard of its own
#ifdef __cplusplus
extern "C" {
#endif
#include "wren.h"
#ifdef __cplusplus
}
#endif

#include <spine/spine.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

// generated from src/advance/bsgl.wren at build time
#include "bsgl.wren.inc"

//=====================================================================
// VM state
//=====================================================================

class bsglWrenImpl {
public:
    WrenVM*     vm;
    BSGL*       bsgl;
    char        err[1024];
    bool        vmError;

    // handles for the frame bridge
    WrenHandle* mainHandle;
    WrenHandle* initHandle;
    WrenHandle* updateHandle;
    WrenHandle* renderHandle;

    // shared call handles for deferred callbacks
    WrenHandle* call0;
    WrenHandle* call1;
    WrenHandle* call2;
    WrenHandle* wOnRender;
    WrenHandle* wOnOver;
    WrenHandle* wOnDown;
    WrenHandle* wOnMove;
    WrenHandle* wOnUp;
};

static bsglWrenImpl* S = nullptr;   // the active host (single VM per process)
static bool g_quitRequested = false;

//=====================================================================
// Deferred Wren calls (see the comment at the top of this file)
//=====================================================================

struct DeferredCall {
    WrenHandle* recv;       // receiver: a widget instance or a Fn value
    WrenHandle* method;     // call handle ("call(_)" / "onDown" / ...)
    double      d0, d1;     // numeric arguments
    const char* str;        // string argument (malloc'ed copy) or null
    bool        bflag;      // bool argument (OnUp(inside))
    int         nargs;      // 0..2 numeric args set
    bool        useStr;
    bool        useBool;
};

static std::vector<DeferredCall> g_deferred;

static void Defer(WrenHandle* recv, WrenHandle* method,
                  double d0 = 0.0, double d1 = 0.0, int nargs = 0,
                  const char* str = nullptr, bool bflag = false) {
    DeferredCall dc;
    dc.recv = recv;
    dc.method = method;
    dc.d0 = d0;
    dc.d1 = d1;
    dc.nargs = nargs;
    dc.str = str ? _strdup(str) : nullptr;
    dc.bflag = bflag;
    dc.useStr = str != nullptr;
    dc.useBool = false;
    g_deferred.push_back(dc);
}

static void DeferBool(WrenHandle* recv, WrenHandle* method, bool bflag) {
    DeferredCall dc;
    dc.recv = recv;
    dc.method = method;
    dc.d0 = dc.d1 = 0.0;
    dc.nargs = 0;
    dc.str = nullptr;
    dc.bflag = bflag;
    dc.useStr = false;
    dc.useBool = true;
    g_deferred.push_back(dc);
}

static void DrainDeferred() {
    if (g_deferred.empty() || !S || !S->vm) {
        return;
    }

    wrenEnsureSlots(S->vm, 3);

    std::vector<DeferredCall> calls;
    calls.swap(g_deferred);

    for (size_t i = 0; i < calls.size(); ++i) {
        DeferredCall& dc = calls[i];
        wrenSetSlotHandle(S->vm, 0, dc.recv);
        int slot = 1;
        if (dc.useStr) {
            wrenSetSlotString(S->vm, slot++, dc.str);
        }
        if (dc.useBool) {
            wrenSetSlotBool(S->vm, slot++, dc.bflag);
        }
        for (int a = 0; a < dc.nargs; ++a) {
            wrenSetSlotDouble(S->vm, slot++, a == 0 ? dc.d0 : dc.d1);
        }
        if (wrenCall(S->vm, dc.method) != WREN_RESULT_SUCCESS) {
            S->bsgl->System_Log("wren: error in a deferred callback");
        }
        if (dc.str) {
            free((void*)dc.str);
        }
    }
}

//=====================================================================
// Foreign-class wrappers
//=====================================================================

struct TexWrap {
    HTEXTURE h;
    bool     owned;     // free the handle in finalize
};

struct SpriteWrap {
    bsglSprite* s;
};

struct AnimWrap {
    bsglAnimation* a;
};

struct QuadWrap {
    bsglQuad q;
};

struct FontWrap {
    bsglFont*   f;
    HTEXTURE    tex;        // scratch texture for Font.render
    bsglSprite* spr;
    int         size;
    int         w, h;
    int         lineH;
    int         baseY;
};

struct SpineWrap {
    bsglSpine*   sp;
    bool         loaded;
    WrenHandle*  ev;        // Fn value or null
    WrenHandle*  cmp;
};

struct DBonesWrap {
    bsglDBones* db;
    bool        loaded;
};

struct WidgetWrap {
    class WrenWidget* w;
};

//---------------------------------------------------------------------
// WrenWidget: bridges the bsglWidget virtuals into Wren methods by
// deferring the calls (they fire inside foreign methods)
//---------------------------------------------------------------------

class WrenWidget : public bsglWidget {
public:
    WrenWidget(int x, int y, int w, int h)
        : bsglWidget(x, y, w, h), self(nullptr) {}

    ~WrenWidget() {
        if (self) {
            wrenReleaseHandle(S->vm, self);
        }
    }

    void BindSelf(WrenVM* vm) {
        if (self) {
            wrenReleaseHandle(vm, self);
        }
        // slot 0 is the receiver of the foreign _init() call
        self = wrenGetSlotHandle(vm, 0);
    }

    void OnRender(float x, float y) {
        bsglWidget::OnRender(x, y);    // draw the background quad
        if (self) Defer(self, S->wOnRender, x, y, 2);
    }

    void OnOver(float x, float y) {
        if (self) Defer(self, S->wOnOver, x, y, 2);
    }

    void OnDown() {
        if (self) Defer(self, S->wOnDown);
    }

    void OnMove(float dx, float dy) {
        if (self) Defer(self, S->wOnMove, dx, dy, 2);
    }

    void OnUp(bool inside) {
        if (self) DeferBool(self, S->wOnUp, inside);
    }

private:
    WrenHandle* self;
};

//---------------------------------------------------------------------
// Spine callbacks (fire inside bsglSpine::Update): deferred
//---------------------------------------------------------------------

static void SpineEventCb(spine::Event* event, void* userdata) {
    SpineWrap* w = (SpineWrap*)userdata;
    if (!event || !w || !w->ev || !S) {
        return;
    }
    const char* name = "";
    const char* sval = event->getStringValue().buffer();
    if (sval && sval[0]) {
        name = sval;
    } else {
        name = event->getData().getName().buffer();
    }
    // invoked as fn(name, time)
    Defer(w->ev, S->call2, event->getTime(), 0.0, 1, name, false);
}

static void SpineCompleteCb(int trackIndex, int loopCount, void* userdata) {
    SpineWrap* w = (SpineWrap*)userdata;
    if (!w || !w->cmp || !S) {
        return;
    }
    Defer(w->cmp, S->call2, (double)trackIndex, (double)loopCount, 2);
}

//=====================================================================
// Slot access helpers
//=====================================================================

static const char* ArgStr(int slot)      { return wrenGetSlotString(S->vm, slot); }
static double      ArgD(int slot)        { return wrenGetSlotDouble(S->vm, slot); }
static int         ArgI(int slot)        { return (int)ArgD(slot); }
static float       ArgF(int slot)        { return (float)ArgD(slot); }
static bool        ArgB(int slot)        { return wrenGetSlotBool(S->vm, slot); }
static DWORD       ArgColor(int slot)    { return (DWORD)(unsigned long long)ArgD(slot); }

static HTEXTURE SlotTexture(int slot) {
    if (wrenGetSlotType(S->vm, slot) == WREN_TYPE_NULL) {
        return 0;
    }
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, slot);
    return t ? t->h : 0;
}

//=====================================================================
// System
//=====================================================================

static void SysSetTitle(WrenVM*)    { S->bsgl->System_SetStateString(BSGL_TITLE, ArgStr(1)); }
static void SysSetLogFile(WrenVM*)  { S->bsgl->System_SetStateString(BSGL_LOGFILE, ArgStr(1)); }
static void SysSetCfgFile(WrenVM*)  { S->bsgl->System_SetStateString(BSGL_CFGFILE, ArgStr(1)); }
static void SysSetWindowed(WrenVM*) { S->bsgl->System_SetStateBool(BSGL_WINDOWED, ArgB(1)); }
static void SysSetWidth(WrenVM*)    { S->bsgl->System_SetStateInt(BSGL_SCREENWIDTH, ArgI(1)); }
static void SysSetHeight(WrenVM*)   { S->bsgl->System_SetStateInt(BSGL_SCREENHEIGHT, ArgI(1)); }
static void SysGetWidth(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, S->bsgl->System_GetStateInt(BSGL_SCREENWIDTH)); }
static void SysGetHeight(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, S->bsgl->System_GetStateInt(BSGL_SCREENHEIGHT)); }
static void SysLog(WrenVM*)         { S->bsgl->System_Log("%s", ArgStr(1)); }
static void SysQuit(WrenVM*)        { g_quitRequested = true; }
static void SysErrMsg(WrenVM*)      { wrenSetSlotString(S->vm, 0, S->bsgl->System_GetErrorMessage()); }

//=====================================================================
// Config / Timer / Random / Input / Gfx statics
//=====================================================================

static void CfgGetInt(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, S->bsgl->Config_GetInt(ArgStr(1), ArgStr(2), ArgI(3))); }
static void CfgSetInt(WrenVM*)   { S->bsgl->Config_SetInt(ArgStr(1), ArgStr(2), ArgI(3)); }
static void CfgGetFloat(WrenVM*) { wrenSetSlotDouble(S->vm, 0, S->bsgl->Config_GetFloat(ArgStr(1), ArgStr(2), ArgF(3))); }
static void CfgSetFloat(WrenVM*) { S->bsgl->Config_SetFloat(ArgStr(1), ArgStr(2), ArgF(3)); }
static void CfgGetStr(WrenVM*)   { wrenSetSlotString(S->vm, 0, S->bsgl->Config_GetString(ArgStr(1), ArgStr(2), ArgStr(3))); }
static void CfgSetStr(WrenVM*)   { S->bsgl->Config_SetString(ArgStr(1), ArgStr(2), ArgStr(3)); }

static void TmrTime(WrenVM*)  { wrenSetSlotDouble(S->vm, 0, S->bsgl->Timer_GetTime()); }
static void TmrDelta(WrenVM*) { wrenSetSlotDouble(S->vm, 0, S->bsgl->Timer_GetDelta()); }
static void TmrFps(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, S->bsgl->Timer_GetFPS()); }

static void RndSeed(WrenVM*)   { S->bsgl->Random_Seed(ArgI(1)); }
static void RndInt(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, S->bsgl->Random_Int(ArgI(1), ArgI(2))); }
static void RndFloat(WrenVM*)  { wrenSetSlotDouble(S->vm, 0, S->bsgl->Random_Float(ArgF(1), ArgF(2))); }

static void InpUpdate(WrenVM*)  { S->bsgl->Control_GetState(); }
static void InpIsDown(WrenVM*)  { wrenSetSlotBool(S->vm, 0, S->bsgl->Control_IsDown(ArgI(1))); }
static void InpIsPass(WrenVM*)  { wrenSetSlotBool(S->vm, 0, S->bsgl->Control_IsPassing(ArgI(1))); }
static void InpIsUp(WrenVM*)    { wrenSetSlotBool(S->vm, 0, S->bsgl->Control_IsUp(ArgI(1))); }
static void InpMouseX(WrenVM*)  { wrenSetSlotDouble(S->vm, 0, S->bsgl->Control_GetMouseX()); }
static void InpMouseY(WrenVM*)  { wrenSetSlotDouble(S->vm, 0, S->bsgl->Control_GetMouseY()); }

static void GfxBegin(WrenVM*)   { S->bsgl->Gfx_BeginScene(); }
static void GfxEnd(WrenVM*)     { S->bsgl->Gfx_EndScene(); }
static void GfxClear(WrenVM*)   { S->bsgl->Gfx_Clear(ArgColor(1)); }
static void GfxClip(WrenVM*)    { S->bsgl->Gfx_SetClipping(ArgI(1), ArgI(2), ArgI(3), ArgI(4)); }
static void GfxXform(WrenVM*)   { S->bsgl->Gfx_SetTransform(ArgF(1), ArgF(2), ArgF(3), ArgF(4), ArgF(5), ArgF(6), ArgF(7)); }
static void GfxQuad(WrenVM*) {
    QuadWrap* q = (QuadWrap*)wrenGetSlotForeign(S->vm, 1);
    if (q) {
        S->bsgl->Gfx_RenderQuad(&q->q);
    }
}

//=====================================================================
// Texture
//=====================================================================

static void TexAllocate(WrenVM* vm) {
    TexWrap* t = (TexWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(TexWrap));
    t->h = 0;
    t->owned = false;
}

static void TexFinalize(void* data) {
    TexWrap* t = (TexWrap*)data;
    if (t->owned && t->h) {
        S->bsgl->Texture_Free(t->h);
    }
    t->h = 0;
    t->owned = false;
}

static void TexLoad(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    t->h = S->bsgl->Texture_Load(ArgStr(1));
    t->owned = (t->h != 0);
    wrenSetSlotBool(S->vm, 0, t->h != 0);
}

static void TexCreate(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    t->h = S->bsgl->Texture_Create(ArgI(1), ArgI(2));
    t->owned = (t->h != 0);
    wrenSetSlotBool(S->vm, 0, t->h != 0);
}

static void TexLoaded(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    wrenSetSlotBool(S->vm, 0, t->h != 0);
}

static void TexWidth(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    wrenSetSlotDouble(S->vm, 0, t->h ? S->bsgl->Texture_GetWidth(t->h) : 0);
}

static void TexHeight(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    wrenSetSlotDouble(S->vm, 0, t->h ? S->bsgl->Texture_GetHeight(t->h) : 0);
}

static void TexFree(WrenVM*) {
    TexWrap* t = (TexWrap*)wrenGetSlotForeign(S->vm, 0);
    if (t->owned && t->h) {
        S->bsgl->Texture_Free(t->h);
    }
    t->h = 0;
    t->owned = false;
}

//=====================================================================
// Quad
//=====================================================================

static void QuadAllocate(WrenVM* vm) {
    QuadWrap* q = (QuadWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(QuadWrap));
    memset(&q->q, 0, sizeof(q->q));
}

static void QuadFinalize(void*) {}

static void QuadSetTexture(WrenVM*) {
    QuadWrap* q = (QuadWrap*)wrenGetSlotForeign(S->vm, 0);
    q->q.tex = SlotTexture(1);
}

static void QuadSetBlend(WrenVM*) {
    QuadWrap* q = (QuadWrap*)wrenGetSlotForeign(S->vm, 0);
    q->q.blend = ArgI(1);
}

static void QuadSetVertex(WrenVM*) {
    QuadWrap* q = (QuadWrap*)wrenGetSlotForeign(S->vm, 0);
    int i = ArgI(1);
    if (i < 0 || i > 3) {
        return;
    }
    bsglVertex& v = q->q.v[i];
    v.tx = ArgF(2);
    v.ty = ArgF(3);
    v.x  = ArgF(4);
    v.y  = ArgF(5);
    v.z  = ArgF(6);
    v.color = ArgColor(7);
}

//=====================================================================
// Sprite
//=====================================================================

static void SprAllocate(WrenVM* vm) {
    SpriteWrap* w = (SpriteWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(SpriteWrap));
    w->s = nullptr;
}

static void SprFinalize(void* data) {
    SpriteWrap* w = (SpriteWrap*)data;
    delete w->s;
    w->s = nullptr;
}

static void SprInit(WrenVM*) {
    SpriteWrap* w = (SpriteWrap*)wrenGetSlotForeign(S->vm, 0);
    w->s = new bsglSprite(SlotTexture(1), ArgF(2), ArgF(3), ArgF(4), ArgF(5));
}

static void SprRender(WrenVM*)   { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->Render(ArgF(1), ArgF(2)); }
static void SprRenderEx3(WrenVM*) {
    ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->RenderEx(ArgF(1), ArgF(2), ArgF(3));
}
static void SprRenderEx5(WrenVM*) {
    ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->RenderEx(ArgF(1), ArgF(2), ArgF(3), ArgF(4), ArgF(5));
}
static void SprStretch(WrenVM*)  { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->RenderStretch(ArgF(1), ArgF(2), ArgF(3), ArgF(4)); }
static void SprRender4V(WrenVM*) { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->Render4V(ArgF(1), ArgF(2), ArgF(3), ArgF(4), ArgF(5), ArgF(6), ArgF(7), ArgF(8)); }
static void SprSetTex(WrenVM*)   { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetTexture(SlotTexture(1)); }
static void SprSetRect(WrenVM*)  { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetTextureRect(ArgF(1), ArgF(2), ArgF(3), ArgF(4)); }
static void SprSetColor(WrenVM*) { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetColor(ArgColor(1)); }
static void SprSetZ(WrenVM*)     { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetZ(ArgF(1)); }
static void SprSetBlend(WrenVM*) { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetBlendMode(ArgI(1)); }
static void SprSetHot(WrenVM*)   { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetHotSpot(ArgF(1), ArgF(2)); }
static void SprSetFlip(WrenVM*)  { ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->SetFlip(ArgB(1), ArgB(2)); }
static void SprWidth(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->GetWidth()); }
static void SprHeight(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, ((SpriteWrap*)wrenGetSlotForeign(S->vm, 0))->s->GetHeight()); }

//=====================================================================
// Animation
//=====================================================================

static void AnimAllocate(WrenVM* vm) {
    AnimWrap* w = (AnimWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(AnimWrap));
    w->a = nullptr;
}

static void AnimFinalize(void* data) {
    AnimWrap* w = (AnimWrap*)data;
    delete w->a;
    w->a = nullptr;
}

static void AnimInit(WrenVM*) {
    AnimWrap* w = (AnimWrap*)wrenGetSlotForeign(S->vm, 0);
    w->a = new bsglAnimation(SlotTexture(1), ArgI(2), ArgF(3), ArgF(4), ArgF(5), ArgF(6), ArgF(7));
}

static bsglAnimation* AnimSelf() { return ((AnimWrap*)wrenGetSlotForeign(S->vm, 0))->a; }

static void AnimPlay(WrenVM*)    { AnimSelf()->Play(); }
static void AnimStop(WrenVM*)    { AnimSelf()->Stop(); }
static void AnimResume(WrenVM*)  { AnimSelf()->Resume(); }
static void AnimUpdate(WrenVM*)  { AnimSelf()->Update(ArgF(1)); }
static void AnimPlaying(WrenVM*) { wrenSetSlotBool(S->vm, 0, AnimSelf()->IsPlaying()); }
static void AnimSetTex(WrenVM*)  { AnimSelf()->SetTexture(SlotTexture(1)); }
static void AnimSetRect(WrenVM*) { AnimSelf()->SetTextureRect(ArgF(1), ArgF(2), ArgF(3), ArgF(4)); }
static void AnimSetMode(WrenVM*) { AnimSelf()->SetMode(ArgI(1)); }
static void AnimSetSpeed(WrenVM*){ AnimSelf()->SetSpeed(ArgF(1)); }
static void AnimSetFrame(WrenVM*){ AnimSelf()->SetFrame(ArgI(1)); }
static void AnimSetFrames(WrenVM*){ AnimSelf()->SetFrames(ArgI(1)); }
static void AnimMode(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetMode()); }
static void AnimSpeed(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetSpeed()); }
static void AnimFrame(WrenVM*)   { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetFrame()); }
static void AnimFrames(WrenVM*)  { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetFrames()); }
static void AnimRender(WrenVM*)  { AnimSelf()->Render(ArgF(1), ArgF(2)); }
static void AnimRenderEx3(WrenVM*) { AnimSelf()->RenderEx(ArgF(1), ArgF(2), ArgF(3)); }
static void AnimRenderEx5(WrenVM*) { AnimSelf()->RenderEx(ArgF(1), ArgF(2), ArgF(3), ArgF(4), ArgF(5)); }
static void AnimSetColor(WrenVM*)  { AnimSelf()->SetColor(ArgColor(1)); }
static void AnimSetBlend(WrenVM*)  { AnimSelf()->SetBlendMode(ArgI(1)); }
static void AnimSetHot(WrenVM*)    { AnimSelf()->SetHotSpot(ArgF(1), ArgF(2)); }
static void AnimSetFlip(WrenVM*)   { AnimSelf()->SetFlip(ArgB(1), ArgB(2)); }
static void AnimWidth(WrenVM*)     { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetWidth()); }
static void AnimHeight(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, AnimSelf()->GetHeight()); }

//=====================================================================
// Font
//=====================================================================

static void FontAllocate(WrenVM* vm) {
    FontWrap* w = (FontWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(FontWrap));
    w->f = nullptr;
    w->tex = 0;
    w->spr = nullptr;
    w->size = 0;
    w->w = w->h = 0;
    w->lineH = w->baseY = 0;
}

static void FontFinalize(void* data) {
    FontWrap* w = (FontWrap*)data;
    delete w->spr;
    w->spr = nullptr;
    if (w->tex) {
        S->bsgl->Texture_Free(w->tex);
        w->tex = 0;
    }
    delete w->f;
    w->f = nullptr;
}

static void FontInit(WrenVM*) {
    FontWrap* w = (FontWrap*)wrenGetSlotForeign(S->vm, 0);
    w->size = ArgI(2);
    w->f = new bsglFont(ArgStr(1), w->size);
    w->lineH = (w->size * 3) / 2;
    w->baseY = w->size + w->lineH / 6;
}

static void FontLoaded(WrenVM*) {
    FontWrap* w = (FontWrap*)wrenGetSlotForeign(S->vm, 0);
    wrenSetSlotBool(S->vm, 0, w->f != nullptr);
}

static void FontRender(WrenVM*) {
    FontWrap* w = (FontWrap*)wrenGetSlotForeign(S->vm, 0);
    if (!w->f) {
        return;
    }

    // lazy scratch texture (must be created after System_Initiate)
    if (!w->tex) {
        w->w = 512;
        w->h = w->lineH + 8;
        w->tex = S->bsgl->Texture_Create(w->w, w->h);
        w->spr = new bsglSprite(w->tex, 0, 0, (float)w->w, (float)w->h);
    }

    // clear to transparent, then draw the glyphs (the same flow as
    // tutorial04: new text would otherwise blend over the old one)
    DWORD* pixels = S->bsgl->Texture_LoadData(w->tex);
    if (pixels) {
        memset(pixels, 0, w->w * w->h * sizeof(DWORD));
        S->bsgl->Texture_Update(w->tex, pixels, 0, 0, w->w, w->h);
        S->bsgl->Texture_FreeData(pixels);
    }

    const char* text = ArgStr(3);
    w->f->BeginDrawTexture(w->tex, 2, w->baseY, w->lineH);
    for (const char* p = text; *p; ++p) {
        w->f->DrawGlyph((wchar_t)(unsigned char)*p);
    }
    w->f->EndDrawTexture();

    w->spr->SetColor(ArgColor(4));
    w->spr->Render(ArgF(1), ArgF(2));
}

//=====================================================================
// Spine
//=====================================================================

static void SpineAllocate(WrenVM* vm) {
    SpineWrap* w = (SpineWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(SpineWrap));
    w->sp = nullptr;
    w->loaded = false;
    w->ev = nullptr;
    w->cmp = nullptr;
}

static void SpineFinalize(void* data) {
    SpineWrap* w = (SpineWrap*)data;
    if (w->ev) {
        wrenReleaseHandle(S->vm, w->ev);
        w->ev = nullptr;
    }
    if (w->cmp) {
        wrenReleaseHandle(S->vm, w->cmp);
        w->cmp = nullptr;
    }
    delete w->sp;
    w->sp = nullptr;
}

static void SpineInit(WrenVM*) {
    SpineWrap* w = (SpineWrap*)wrenGetSlotForeign(S->vm, 0);
    w->sp = new bsglSpine();
    w->sp->SetEventCallback(SpineEventCb, w);
    w->sp->SetCompleteCallback(SpineCompleteCb, w);
    w->loaded = w->sp->Load(ArgStr(1), ArgStr(2));
    wrenSetSlotBool(S->vm, 0, w->loaded);
}

static bsglSpine* SpineSelf() { return ((SpineWrap*)wrenGetSlotForeign(S->vm, 0))->sp; }

static void SpineLoaded(WrenVM*)  { wrenSetSlotBool(S->vm, 0, ((SpineWrap*)wrenGetSlotForeign(S->vm, 0))->loaded); }
static void SpineSetAnim(WrenVM*) { SpineSelf()->SetAnimation(ArgI(1), ArgStr(2), ArgB(3)); }
static void SpineAddAnim(WrenVM*) { SpineSelf()->AddAnimation(ArgI(1), ArgStr(2), ArgB(3), ArgF(4)); }
static void SpineSetEmpty(WrenVM*){ SpineSelf()->SetEmptyAnimation(ArgI(1), ArgF(2)); }
static void SpineClearTrk(WrenVM*){ SpineSelf()->ClearTrack(ArgI(1)); }
static void SpineClearTrks(WrenVM*){ SpineSelf()->ClearTracks(); }
static void SpineCurAnim(WrenVM*) {
    const char* n = SpineSelf()->GetCurrentAnimation();
    wrenSetSlotString(S->vm, 0, n ? n : "");
}
static void SpinePlaying(WrenVM*)  { wrenSetSlotBool(S->vm, 0, SpineSelf()->IsPlaying()); }
static void SpineSetTS(WrenVM*)    { SpineSelf()->SetTimeScale((float)ArgD(1)); }
static void SpineGetTS(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, SpineSelf()->GetTimeScale()); }
static void SpineDefMix(WrenVM*)   { SpineSelf()->SetDefaultMix(ArgF(1)); }
static void SpineMix(WrenVM*)      { SpineSelf()->SetMix(ArgStr(1), ArgStr(2), ArgF(3)); }
static void SpineSetPos(WrenVM*)   { SpineSelf()->SetPosition(ArgF(1), ArgF(2)); }
static void SpineX(WrenVM*)        { float x, y; SpineSelf()->GetPosition(&x, &y); wrenSetSlotDouble(S->vm, 0, x); }
static void SpineY(WrenVM*)        { float x, y; SpineSelf()->GetPosition(&x, &y); wrenSetSlotDouble(S->vm, 0, y); }
static void SpineSetRot(WrenVM*)   { SpineSelf()->SetRotation(ArgF(1)); }
static void SpineRot(WrenVM*)      { wrenSetSlotDouble(S->vm, 0, SpineSelf()->GetRotation()); }
static void SpineSetScale(WrenVM*) { SpineSelf()->SetScale(ArgF(1), ArgF(2)); }
static void SpineSetFlip(WrenVM*)  { SpineSelf()->SetFlip(ArgB(1), ArgB(2)); }
static void SpineSetColor(WrenVM*) { SpineSelf()->SetColor(ArgColor(1)); }
static void SpineColor(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, (double)(unsigned long long)SpineSelf()->GetColor()); }
static void SpineUpdate(WrenVM*)   { SpineSelf()->Update(ArgF(1)); }
static void SpineRender(WrenVM*)   { SpineSelf()->Render(); }
static void SpineHit(WrenVM*)      { wrenSetSlotBool(S->vm, 0, SpineSelf()->HitTest(ArgF(1), ArgF(2))); }

static void SpineOnEvent(WrenVM*) {
    SpineWrap* w = (SpineWrap*)wrenGetSlotForeign(S->vm, 0);
    if (w->ev) {
        wrenReleaseHandle(S->vm, w->ev);
    }
    w->ev = wrenGetSlotType(S->vm, 1) == WREN_TYPE_NULL ? nullptr : wrenGetSlotHandle(S->vm, 1);
}

static void SpineOnComplete(WrenVM*) {
    SpineWrap* w = (SpineWrap*)wrenGetSlotForeign(S->vm, 0);
    if (w->cmp) {
        wrenReleaseHandle(S->vm, w->cmp);
    }
    w->cmp = wrenGetSlotType(S->vm, 1) == WREN_TYPE_NULL ? nullptr : wrenGetSlotHandle(S->vm, 1);
}

//=====================================================================
// DBones
//=====================================================================

static void DBAllocate(WrenVM* vm) {
    DBonesWrap* w = (DBonesWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(DBonesWrap));
    w->db = nullptr;
    w->loaded = false;
}

static void DBFinalize(void* data) {
    DBonesWrap* w = (DBonesWrap*)data;
    delete w->db;
    w->db = nullptr;
}

static void DBInit(WrenVM*) {
    DBonesWrap* w = (DBonesWrap*)wrenGetSlotForeign(S->vm, 0);
    w->db = new bsglDBones();
    const char* arm = wrenGetSlotType(S->vm, 5) == WREN_TYPE_NULL ? nullptr : ArgStr(5);
    w->loaded = w->db->Load(ArgStr(1), ArgStr(2), ArgStr(3), ArgStr(4), arm);
    wrenSetSlotBool(S->vm, 0, w->loaded);
}

static bsglDBones* DBSelf() { return ((DBonesWrap*)wrenGetSlotForeign(S->vm, 0))->db; }

static void DBLoaded(WrenVM*)   { wrenSetSlotBool(S->vm, 0, ((DBonesWrap*)wrenGetSlotForeign(S->vm, 0))->loaded); }
static void DBPlay(WrenVM*)     { DBSelf()->Play(ArgStr(1), -1); }
static void DBPlayTimes(WrenVM*){ DBSelf()->Play(ArgStr(1), ArgI(2)); }
static void DBFadeIn(WrenVM*)   { DBSelf()->FadeIn(ArgStr(1), ArgF(2)); }
static void DBHasAnim(WrenVM*)  { wrenSetSlotBool(S->vm, 0, DBSelf()->HasAnimation(ArgStr(1))); }
static void DBAnimCount(WrenVM*){ wrenSetSlotDouble(S->vm, 0, DBSelf()->GetAnimationCount()); }
static void DBAnimName(WrenVM*) {
    const char* n = DBSelf()->GetAnimationName(ArgI(1));
    wrenSetSlotString(S->vm, 0, n ? n : "");
}
static void DBCurAnim(WrenVM*) {
    const char* n = DBSelf()->GetCurrentAnimation();
    wrenSetSlotString(S->vm, 0, n ? n : "");
}
static void DBSetTS(WrenVM*)    { DBSelf()->SetTimeScale((float)ArgD(1)); }
static void DBGetTS(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, DBSelf()->GetTimeScale()); }
static void DBSetPos(WrenVM*)   { DBSelf()->SetPosition(ArgF(1), ArgF(2)); }
static void DBX(WrenVM*)        { float x, y; DBSelf()->GetPosition(&x, &y); wrenSetSlotDouble(S->vm, 0, x); }
static void DBY(WrenVM*)        { float x, y; DBSelf()->GetPosition(&x, &y); wrenSetSlotDouble(S->vm, 0, y); }
static void DBSetScale(WrenVM*) { DBSelf()->SetScale(ArgF(1), ArgF(2)); }
static void DBSetFlip(WrenVM*)  { DBSelf()->SetFlip(ArgB(1), ArgB(2)); }
static void DBSetColor(WrenVM*) { DBSelf()->SetColor(ArgColor(1)); }
static void DBColor(WrenVM*)    { wrenSetSlotDouble(S->vm, 0, (double)(unsigned long long)DBSelf()->GetColor()); }
static void DBUpdate(WrenVM*)   { DBSelf()->Update(ArgF(1)); }
static void DBRender(WrenVM*)   { DBSelf()->Render(); }

//=====================================================================
// Widget
//=====================================================================

static void WidAllocate(WrenVM* vm) {
    WidgetWrap* w = (WidgetWrap*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WidgetWrap));
    w->w = nullptr;
}

static void WidFinalize(void* data) {
    WidgetWrap* w = (WidgetWrap*)data;
    delete w->w;
    w->w = nullptr;
}

static void WidInit(WrenVM*) {
    WidgetWrap* w = (WidgetWrap*)wrenGetSlotForeign(S->vm, 0);
    w->w = new WrenWidget(ArgI(1), ArgI(2), ArgI(3), ArgI(4));
    // remember the receiver so virtual calls can be deferred to it
    w->w->BindSelf(S->vm);
}

static WrenWidget* WidSelf() { return ((WidgetWrap*)wrenGetSlotForeign(S->vm, 0))->w; }

static void WidSetX(WrenVM*)  { WidSelf()->SetX(ArgI(1)); }
static void WidSetY(WrenVM*)  { WidSelf()->SetY(ArgI(1)); }
static void WidSetBg(WrenVM*) { WidSelf()->SetBackgroundColor(ArgColor(1)); }
static void WidAddKid(WrenVM*) {
    WrenWidget* kid = wrenGetSlotType(S->vm, 1) == WREN_TYPE_NULL
        ? nullptr : ((WidgetWrap*)wrenGetSlotForeign(S->vm, 1))->w;
    if (kid) {
        WidSelf()->AddKid(kid);
    }
}
static void WidRender(WrenVM*)  { WidSelf()->Render(ArgF(1), ArgF(2)); }
static void WidMouseAt(WrenVM*) { WidSelf()->MouseAt(ArgF(1), ArgF(2), (MouseState)ArgI(3)); }
static void WidTestAt(WrenVM*)  { wrenSetSlotBool(S->vm, 0, WidSelf()->TestAt(ArgF(1), ArgF(2))); }

//=====================================================================
// Foreign method / class binding
//=====================================================================

static bool Match(const char* cls, bool isStatic, const char* sig,
                  const char* c, bool st, const char* s) {
    return isStatic == st && strcmp(cls, c) == 0 && strcmp(sig, s) == 0;
}

static WrenForeignMethodFn BindForeignMethod(WrenVM*,
                                             const char* module,
                                             const char* className,
                                             bool isStatic,
                                             const char* signature) {
    if (strcmp(module, "bsgl") != 0) {
        return nullptr;
    }

    // --- System -----------------------------------------------------
    if (Match(className, isStatic, signature, "System", true, "title=(_)"))         return SysSetTitle;
    if (Match(className, isStatic, signature, "System", true, "logFile=(_)"))       return SysSetLogFile;
    if (Match(className, isStatic, signature, "System", true, "cfgFile=(_)"))       return SysSetCfgFile;
    if (Match(className, isStatic, signature, "System", true, "windowed=(_)"))      return SysSetWindowed;
    if (Match(className, isStatic, signature, "System", true, "width=(_)"))         return SysSetWidth;
    if (Match(className, isStatic, signature, "System", true, "height=(_)"))        return SysSetHeight;
    if (Match(className, isStatic, signature, "System", true, "width"))             return SysGetWidth;
    if (Match(className, isStatic, signature, "System", true, "height"))            return SysGetHeight;
    if (Match(className, isStatic, signature, "System", true, "log(_)"))            return SysLog;
    if (Match(className, isStatic, signature, "System", true, "quit"))              return SysQuit;
    if (Match(className, isStatic, signature, "System", true, "errorMessage"))      return SysErrMsg;

    // --- Config -----------------------------------------------------
    if (Match(className, isStatic, signature, "Config", true, "getInt(_,_,_)"))     return CfgGetInt;
    if (Match(className, isStatic, signature, "Config", true, "setInt(_,_,_)"))     return CfgSetInt;
    if (Match(className, isStatic, signature, "Config", true, "getFloat(_,_,_)"))   return CfgGetFloat;
    if (Match(className, isStatic, signature, "Config", true, "setFloat(_,_,_)"))   return CfgSetFloat;
    if (Match(className, isStatic, signature, "Config", true, "getString(_,_,_)"))  return CfgGetStr;
    if (Match(className, isStatic, signature, "Config", true, "setString(_,_,_)"))  return CfgSetStr;

    // --- Timer ------------------------------------------------------
    if (Match(className, isStatic, signature, "Timer", true, "time"))               return TmrTime;
    if (Match(className, isStatic, signature, "Timer", true, "delta"))              return TmrDelta;
    if (Match(className, isStatic, signature, "Timer", true, "fps"))                return TmrFps;

    // --- Random -----------------------------------------------------
    if (Match(className, isStatic, signature, "Random", true, "seed(_)"))           return RndSeed;
    if (Match(className, isStatic, signature, "Random", true, "int(_,_)"))          return RndInt;
    if (Match(className, isStatic, signature, "Random", true, "float(_,_)"))        return RndFloat;

    // --- Input ------------------------------------------------------
    if (Match(className, isStatic, signature, "Input", true, "update"))             return InpUpdate;
    if (Match(className, isStatic, signature, "Input", true, "isDown(_)"))          return InpIsDown;
    if (Match(className, isStatic, signature, "Input", true, "isPassing(_)"))       return InpIsPass;
    if (Match(className, isStatic, signature, "Input", true, "isUp(_)"))            return InpIsUp;
    if (Match(className, isStatic, signature, "Input", true, "mouseX"))             return InpMouseX;
    if (Match(className, isStatic, signature, "Input", true, "mouseY"))             return InpMouseY;

    // --- Gfx --------------------------------------------------------
    if (Match(className, isStatic, signature, "Gfx", true, "beginScene"))           return GfxBegin;
    if (Match(className, isStatic, signature, "Gfx", true, "endScene"))             return GfxEnd;
    if (Match(className, isStatic, signature, "Gfx", true, "clear(_)"))             return GfxClear;
    if (Match(className, isStatic, signature, "Gfx", true, "setClipping(_,_,_,_)")) return GfxClip;
    if (Match(className, isStatic, signature, "Gfx", true, "setTransform(_,_,_,_,_,_,_)")) return GfxXform;
    if (Match(className, isStatic, signature, "Gfx", true, "renderQuadRaw(_)"))     return GfxQuad;

    // --- Quad -------------------------------------------------------
    if (Match(className, isStatic, signature, "Quad", true, "allocate"))            return QuadAllocate;
    if (Match(className, isStatic, signature, "Quad", false, "setTexture(_)"))      return QuadSetTexture;
    if (Match(className, isStatic, signature, "Quad", false, "blend=(_)"))          return QuadSetBlend;
    if (Match(className, isStatic, signature, "Quad", false, "setVertex(_,_,_,_,_,_,_)")) return QuadSetVertex;

    // --- Texture ----------------------------------------------------
    if (Match(className, isStatic, signature, "Texture", true, "allocate"))         return TexAllocate;
    if (Match(className, isStatic, signature, "Texture", false, "_load(_)"))        return TexLoad;
    if (Match(className, isStatic, signature, "Texture", false, "_create(_,_)"))    return TexCreate;
    if (Match(className, isStatic, signature, "Texture", false, "loaded"))          return TexLoaded;
    if (Match(className, isStatic, signature, "Texture", false, "width"))           return TexWidth;
    if (Match(className, isStatic, signature, "Texture", false, "height"))          return TexHeight;
    if (Match(className, isStatic, signature, "Texture", false, "free"))            return TexFree;

    // --- Sprite -----------------------------------------------------
    if (Match(className, isStatic, signature, "Sprite", true, "allocate"))          return SprAllocate;
    if (Match(className, isStatic, signature, "Sprite", false, "_init(_,_,_,_,_)")) return SprInit;
    if (Match(className, isStatic, signature, "Sprite", false, "render(_,_)"))      return SprRender;
    if (Match(className, isStatic, signature, "Sprite", false, "renderEx(_,_,_)"))  return SprRenderEx3;
    if (Match(className, isStatic, signature, "Sprite", false, "renderEx(_,_,_,_,_)")) return SprRenderEx5;
    if (Match(className, isStatic, signature, "Sprite", false, "renderStretch(_,_,_,_)")) return SprStretch;
    if (Match(className, isStatic, signature, "Sprite", false, "render4V(_,_,_,_,_,_,_,_)")) return SprRender4V;
    if (Match(className, isStatic, signature, "Sprite", false, "setTexture(_)"))    return SprSetTex;
    if (Match(className, isStatic, signature, "Sprite", false, "setTextureRect(_,_,_,_)")) return SprSetRect;
    if (Match(className, isStatic, signature, "Sprite", false, "setColor(_)"))      return SprSetColor;
    if (Match(className, isStatic, signature, "Sprite", false, "setZ(_)"))          return SprSetZ;
    if (Match(className, isStatic, signature, "Sprite", false, "setBlendMode(_)"))  return SprSetBlend;
    if (Match(className, isStatic, signature, "Sprite", false, "setHotSpot(_,_)"))  return SprSetHot;
    if (Match(className, isStatic, signature, "Sprite", false, "setFlip(_,_)"))     return SprSetFlip;
    if (Match(className, isStatic, signature, "Sprite", false, "width"))            return SprWidth;
    if (Match(className, isStatic, signature, "Sprite", false, "height"))           return SprHeight;

    // --- Animation --------------------------------------------------
    if (Match(className, isStatic, signature, "Animation", true, "allocate"))       return AnimAllocate;
    if (Match(className, isStatic, signature, "Animation", false, "_init(_,_,_,_,_,_,_)")) return AnimInit;
    if (Match(className, isStatic, signature, "Animation", false, "play"))          return AnimPlay;
    if (Match(className, isStatic, signature, "Animation", false, "stop"))          return AnimStop;
    if (Match(className, isStatic, signature, "Animation", false, "resume"))        return AnimResume;
    if (Match(className, isStatic, signature, "Animation", false, "update(_)"))     return AnimUpdate;
    if (Match(className, isStatic, signature, "Animation", false, "isPlaying"))     return AnimPlaying;
    if (Match(className, isStatic, signature, "Animation", false, "setTexture(_)")) return AnimSetTex;
    if (Match(className, isStatic, signature, "Animation", false, "setTextureRect(_,_,_,_)")) return AnimSetRect;
    if (Match(className, isStatic, signature, "Animation", false, "setMode(_)"))    return AnimSetMode;
    if (Match(className, isStatic, signature, "Animation", false, "setSpeed(_)"))   return AnimSetSpeed;
    if (Match(className, isStatic, signature, "Animation", false, "setFrame(_)"))   return AnimSetFrame;
    if (Match(className, isStatic, signature, "Animation", false, "setFrames(_)"))  return AnimSetFrames;
    if (Match(className, isStatic, signature, "Animation", false, "mode"))          return AnimMode;
    if (Match(className, isStatic, signature, "Animation", false, "speed"))         return AnimSpeed;
    if (Match(className, isStatic, signature, "Animation", false, "frame"))         return AnimFrame;
    if (Match(className, isStatic, signature, "Animation", false, "frames"))        return AnimFrames;
    if (Match(className, isStatic, signature, "Animation", false, "render(_,_)"))   return AnimRender;
    if (Match(className, isStatic, signature, "Animation", false, "renderEx(_,_,_)")) return AnimRenderEx3;
    if (Match(className, isStatic, signature, "Animation", false, "renderEx(_,_,_,_,_)")) return AnimRenderEx5;
    if (Match(className, isStatic, signature, "Animation", false, "setColor(_)"))   return AnimSetColor;
    if (Match(className, isStatic, signature, "Animation", false, "setBlendMode(_)")) return AnimSetBlend;
    if (Match(className, isStatic, signature, "Animation", false, "setHotSpot(_,_)")) return AnimSetHot;
    if (Match(className, isStatic, signature, "Animation", false, "setFlip(_,_)"))  return AnimSetFlip;
    if (Match(className, isStatic, signature, "Animation", false, "width"))         return AnimWidth;
    if (Match(className, isStatic, signature, "Animation", false, "height"))        return AnimHeight;

    // --- Font -------------------------------------------------------
    if (Match(className, isStatic, signature, "Font", true, "allocate"))            return FontAllocate;
    if (Match(className, isStatic, signature, "Font", false, "_init(_,_)"))         return FontInit;
    if (Match(className, isStatic, signature, "Font", false, "loaded"))             return FontLoaded;
    if (Match(className, isStatic, signature, "Font", false, "render(_,_,_,_)"))    return FontRender;

    // --- Spine ------------------------------------------------------
    if (Match(className, isStatic, signature, "Spine", true, "allocate"))           return SpineAllocate;
    if (Match(className, isStatic, signature, "Spine", false, "_init(_,_)"))        return SpineInit;
    if (Match(className, isStatic, signature, "Spine", false, "loaded"))            return SpineLoaded;
    if (Match(className, isStatic, signature, "Spine", false, "setAnimation(_,_,_)"))  return SpineSetAnim;
    if (Match(className, isStatic, signature, "Spine", false, "addAnimation(_,_,_,_)")) return SpineAddAnim;
    if (Match(className, isStatic, signature, "Spine", false, "setEmptyAnimation(_,_)")) return SpineSetEmpty;
    if (Match(className, isStatic, signature, "Spine", false, "clearTrack(_)"))     return SpineClearTrk;
    if (Match(className, isStatic, signature, "Spine", false, "clearTracks"))       return SpineClearTrks;
    if (Match(className, isStatic, signature, "Spine", false, "currentAnimation"))  return SpineCurAnim;
    if (Match(className, isStatic, signature, "Spine", false, "isPlaying"))         return SpinePlaying;
    if (Match(className, isStatic, signature, "Spine", false, "timeScale=(_)"))     return SpineSetTS;
    if (Match(className, isStatic, signature, "Spine", false, "timeScale"))         return SpineGetTS;
    if (Match(className, isStatic, signature, "Spine", false, "setDefaultMix(_)"))  return SpineDefMix;
    if (Match(className, isStatic, signature, "Spine", false, "setMix(_,_,_)"))     return SpineMix;
    if (Match(className, isStatic, signature, "Spine", false, "setPos(_,_)"))       return SpineSetPos;
    if (Match(className, isStatic, signature, "Spine", false, "x"))                 return SpineX;
    if (Match(className, isStatic, signature, "Spine", false, "y"))                 return SpineY;
    if (Match(className, isStatic, signature, "Spine", false, "rotation=(_)"))      return SpineSetRot;
    if (Match(className, isStatic, signature, "Spine", false, "rotation"))          return SpineRot;
    if (Match(className, isStatic, signature, "Spine", false, "setScale(_,_)"))     return SpineSetScale;
    if (Match(className, isStatic, signature, "Spine", false, "setFlip(_,_)"))      return SpineSetFlip;
    if (Match(className, isStatic, signature, "Spine", false, "color=(_)"))         return SpineSetColor;
    if (Match(className, isStatic, signature, "Spine", false, "color"))             return SpineColor;
    if (Match(className, isStatic, signature, "Spine", false, "update(_)"))         return SpineUpdate;
    if (Match(className, isStatic, signature, "Spine", false, "render"))            return SpineRender;
    if (Match(className, isStatic, signature, "Spine", false, "hitTest(_,_)"))      return SpineHit;
    if (Match(className, isStatic, signature, "Spine", false, "onEvent(_)"))        return SpineOnEvent;
    if (Match(className, isStatic, signature, "Spine", false, "onComplete(_)"))     return SpineOnComplete;

    // --- DBones -----------------------------------------------------
    if (Match(className, isStatic, signature, "DBones", true, "allocate"))          return DBAllocate;
    if (Match(className, isStatic, signature, "DBones", false, "_init(_,_,_,_,_)")) return DBInit;
    if (Match(className, isStatic, signature, "DBones", false, "loaded"))           return DBLoaded;
    if (Match(className, isStatic, signature, "DBones", false, "play(_)"))          return DBPlay;
    if (Match(className, isStatic, signature, "DBones", false, "playTimes(_,_)"))   return DBPlayTimes;
    if (Match(className, isStatic, signature, "DBones", false, "fadeIn(_,_)"))      return DBFadeIn;
    if (Match(className, isStatic, signature, "DBones", false, "hasAnimation(_)"))  return DBHasAnim;
    if (Match(className, isStatic, signature, "DBones", false, "animationCount"))   return DBAnimCount;
    if (Match(className, isStatic, signature, "DBones", false, "animationName(_)")) return DBAnimName;
    if (Match(className, isStatic, signature, "DBones", false, "currentAnimation")) return DBCurAnim;
    if (Match(className, isStatic, signature, "DBones", false, "timeScale=(_)"))    return DBSetTS;
    if (Match(className, isStatic, signature, "DBones", false, "timeScale"))        return DBGetTS;
    if (Match(className, isStatic, signature, "DBones", false, "setPos(_,_)"))      return DBSetPos;
    if (Match(className, isStatic, signature, "DBones", false, "x"))                return DBX;
    if (Match(className, isStatic, signature, "DBones", false, "y"))                return DBY;
    if (Match(className, isStatic, signature, "DBones", false, "setScale(_,_)"))    return DBSetScale;
    if (Match(className, isStatic, signature, "DBones", false, "setFlip(_,_,)"))    return DBSetFlip;
    if (Match(className, isStatic, signature, "DBones", false, "setFlip(_,_)"))     return DBSetFlip;
    if (Match(className, isStatic, signature, "DBones", false, "color=(_)"))        return DBSetColor;
    if (Match(className, isStatic, signature, "DBones", false, "color"))            return DBColor;
    if (Match(className, isStatic, signature, "DBones", false, "update(_)"))        return DBUpdate;
    if (Match(className, isStatic, signature, "DBones", false, "render"))           return DBRender;

    // --- Widget -----------------------------------------------------
    if (Match(className, isStatic, signature, "Widget", true, "allocate"))          return WidAllocate;
    if (Match(className, isStatic, signature, "Widget", false, "_init(_,_,_,_)"))   return WidInit;
    if (Match(className, isStatic, signature, "Widget", false, "setX(_)"))          return WidSetX;
    if (Match(className, isStatic, signature, "Widget", false, "setY(_)"))          return WidSetY;
    if (Match(className, isStatic, signature, "Widget", false, "backgroundColor=(_)")) return WidSetBg;
    if (Match(className, isStatic, signature, "Widget", false, "addKid(_)"))        return WidAddKid;
    if (Match(className, isStatic, signature, "Widget", false, "render(_,_)"))      return WidRender;
    if (Match(className, isStatic, signature, "Widget", false, "mouseAt(_,_,_)"))   return WidMouseAt;
    if (Match(className, isStatic, signature, "Widget", false, "testAt(_,_)"))      return WidTestAt;

    return nullptr;
}

static WrenForeignClassMethods BindForeignClass(WrenVM*,
                                                const char* module,
                                                const char* className) {
    WrenForeignClassMethods methods;
    methods.allocate = nullptr;
    methods.finalize = nullptr;

    if (strcmp(module, "bsgl") != 0) {
        return methods;
    }

    if (strcmp(className, "Quad") == 0)    { methods.allocate = QuadAllocate;  methods.finalize = QuadFinalize; }
    if (strcmp(className, "Texture") == 0) { methods.allocate = TexAllocate;   methods.finalize = TexFinalize; }
    if (strcmp(className, "Sprite") == 0)  { methods.allocate = SprAllocate;   methods.finalize = SprFinalize; }
    if (strcmp(className, "Animation") == 0) { methods.allocate = AnimAllocate; methods.finalize = AnimFinalize; }
    if (strcmp(className, "Font") == 0)    { methods.allocate = FontAllocate;  methods.finalize = FontFinalize; }
    if (strcmp(className, "Spine") == 0)   { methods.allocate = SpineAllocate; methods.finalize = SpineFinalize; }
    if (strcmp(className, "DBones") == 0)  { methods.allocate = DBAllocate;    methods.finalize = DBFinalize; }
    if (strcmp(className, "Widget") == 0)  { methods.allocate = WidAllocate;   methods.finalize = WidFinalize; }

    return methods;
}

//=====================================================================
// VM plumbing
//=====================================================================

static void WriteFn(WrenVM*, const char* text) {
    S->bsgl->System_Log("%s", text);
    fputs(text, stdout);
}

static void ErrorFn(WrenVM*, WrenErrorType type,
                    const char* module, int line, const char* message) {
    const char* typeName =
        type == WREN_ERROR_COMPILE ? "compile" :
        type == WREN_ERROR_RUNTIME ? "runtime" : "stack trace";

    if (type == WREN_ERROR_STACK_TRACE) {
        S->bsgl->System_Log("wren: %s: %s:%d", typeName, module ? module : "?", line);
    } else {
        _snprintf(S->err, sizeof(S->err), "%s error [%s:%d]: %s",
                  typeName, module ? module : "?", line, message ? message : "?");
        S->bsgl->System_Log("wren: %s", S->err);
    }
    S->vmError = true;
}

static void FreeModuleSource(WrenVM*, const char*, WrenLoadModuleResult result) {
    free(result.userData);
}

static WrenLoadModuleResult LoadModuleFn(WrenVM*, const char* name) {
    WrenLoadModuleResult result;
    result.source = nullptr;
    result.onComplete = FreeModuleSource;
    result.userData = nullptr;

    char* src = nullptr;
    if (strcmp(name, "bsgl") == 0) {
        // the embedded library
        src = (char*)malloc(strlen(BSGL_WREN_LIB_SOURCE) + 1);
        strcpy(src, BSGL_WREN_LIB_SOURCE);
    } else {
        // user modules: load "<name>.wren" from disk next to the executable
        char path[512];
        _snprintf(path, sizeof(path), "%s.wren", name);
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long len = ftell(f);
            fseek(f, 0, SEEK_SET);
            src = (char*)malloc(len + 1);
            size_t rd = fread(src, 1, len, f);
            fclose(f);
            src[rd] = 0;
        }
    }

    result.source = src;
    result.userData = src;
    return result;
}

//=====================================================================
// Frame bridge
//=====================================================================

static bool WrenLogicFunc() {
    S->bsgl->Control_GetState();
    float dt = S->bsgl->Timer_GetDelta();

    wrenEnsureSlots(S->vm, 2);
    wrenSetSlotHandle(S->vm, 0, S->mainHandle);
    wrenSetSlotDouble(S->vm, 1, dt);
    if (wrenCall(S->vm, S->updateHandle) != WREN_RESULT_SUCCESS) {
        S->bsgl->System_Log("wren: error in main.update(), quitting");
        DrainDeferred();
        return true;
    }
    bool quit = (wrenGetSlotType(S->vm, 0) == WREN_TYPE_BOOL) && wrenGetSlotBool(S->vm, 0);

    DrainDeferred();

    return quit || g_quitRequested;
}

static bool WrenRenderFunc() {
    wrenEnsureSlots(S->vm, 1);
    wrenSetSlotHandle(S->vm, 0, S->mainHandle);
    if (wrenCall(S->vm, S->renderHandle) != WREN_RESULT_SUCCESS) {
        S->bsgl->System_Log("wren: error in main.render(), quitting");
        DrainDeferred();
        return true;
    }

    DrainDeferred();

    return g_quitRequested;
}

//=====================================================================
// bsglWren
//=====================================================================

bsglWren::bsglWren()
    : impl(nullptr) {
    impl = new bsglWrenImpl();
    memset(impl, 0, sizeof(*impl));
    impl->bsgl = bsglCreate(BSGL_VERSION);
    S = impl;
}

bsglWren::~bsglWren() {
    if (impl) {
        if (impl->vm) {
            wrenFreeVM(impl->vm);
        }
        S = nullptr;
        if (impl->bsgl) {
            impl->bsgl->Release();
        }
        delete impl;
    }
}

char const* bsglWren::GetLastError() const {
    return impl->err;
}

static char* ReadWholeFile(char const* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(len + 1);
    size_t rd = fread(buf, 1, len, f);
    fclose(f);
    buf[rd] = 0;
    return buf;
}

bool bsglWren::Run(char const* scriptFile) {
    bsglWrenImpl* I = impl;
    S = I;
    g_quitRequested = false;

    //--- 1. run the script top-level -------------------------------------
    char* source = ReadWholeFile(scriptFile);
    if (!source) {
        _snprintf(I->err, sizeof(I->err), "cannot open script '%s'", scriptFile);
        I->bsgl->System_Log("bsglWren: %s", I->err);
        return false;
    }

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = WriteFn;
    config.errorFn = ErrorFn;
    config.bindForeignMethodFn = BindForeignMethod;
    config.bindForeignClassFn = BindForeignClass;
    config.loadModuleFn = LoadModuleFn;

    I->vm = wrenNewVM(&config);

    WrenInterpretResult result = wrenInterpret(I->vm, "main", source);
    free(source);

    if (result != WREN_RESULT_SUCCESS) {
        return false;
    }

    //--- 2. shared handles for the bridge --------------------------------
    I->call0    = wrenMakeCallHandle(I->vm, "call()");
    I->call1    = wrenMakeCallHandle(I->vm, "call(_)");
    I->call2    = wrenMakeCallHandle(I->vm, "call(_,_)");
    I->wOnRender = wrenMakeCallHandle(I->vm, "onRender(_,_)");
    I->wOnOver   = wrenMakeCallHandle(I->vm, "onOver(_,_)");
    I->wOnDown   = wrenMakeCallHandle(I->vm, "onDown()");
    I->wOnMove   = wrenMakeCallHandle(I->vm, "onMove(_,_)");
    I->wOnUp     = wrenMakeCallHandle(I->vm, "onUp(_)");
    I->initHandle   = wrenMakeCallHandle(I->vm, "init()");
    I->updateHandle = wrenMakeCallHandle(I->vm, "update(_)");
    I->renderHandle = wrenMakeCallHandle(I->vm, "render()");

    //--- 3. fetch the global `main` --------------------------------------
    wrenEnsureSlots(I->vm, 1);
    if (!wrenHasVariable(I->vm, "main", "main")) {
        _snprintf(I->err, sizeof(I->err),
                  "script must define a global 'var main = Main.new()' "
                  "with init()/update(dt)/render()");
        I->bsgl->System_Log("bsglWren: %s", I->err);
        return false;
    }
    wrenGetVariable(I->vm, "main", "main", 0);
    I->mainHandle = wrenGetSlotHandle(I->vm, 0);

    //--- 4. initiate ------------------------------------------------------
    if (!I->bsgl->System_Initiate()) {
        _snprintf(I->err, sizeof(I->err), "System_Initiate: %s",
                  I->bsgl->System_GetErrorMessage());
        I->bsgl->System_Log("bsglWren: %s", I->err);
        return false;
    }

    //--- 5. main.init() ----------------------------------------------------
    wrenEnsureSlots(I->vm, 1);
    wrenSetSlotHandle(I->vm, 0, I->mainHandle);
    if (wrenCall(I->vm, I->initHandle) != WREN_RESULT_SUCCESS) {
        _snprintf(I->err, sizeof(I->err),
                  "main.init() failed (does the main object define init()?)");
        I->bsgl->System_Log("bsglWren: %s", I->err);
        I->bsgl->System_Shutdown();
        return false;
    }
    DrainDeferred();

    //--- 6. the frame loop --------------------------------------------------
    I->bsgl->System_SetStateFunc(BSGL_LOGICFUNC, WrenLogicFunc);
    I->bsgl->System_SetStateFunc(BSGL_RENDERFUNC, WrenRenderFunc);
    I->bsgl->System_Start();

    //--- 7. teardown ----------------------------------------------------------
    // free the VM first: finalizers delete sprites and free textures,
    // which must happen while the GL context is still alive
    DrainDeferred();
    wrenFreeVM(I->vm);
    I->vm = nullptr;
    I->bsgl->System_Shutdown();

    return !I->vmError;
}
