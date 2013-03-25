/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Common core implementation header
*/

#ifndef BSGL_IMPL_H
#define BSGL_IMPL_H

#include "bsgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#if !defined(CC_TARGET_OS_IPHONE)
#include <SDL/SDL.h>
#include <GL/glew.h>
#else
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#endif

#define VERTEX_BUFFER_SIZE 4000

typedef bool (*FrameFunc)();

struct TextureList {
    HTEXTURE            tex;
    int                 width;
    int                 height;
    struct TextureList* next;
#if defined(CC_TARGET_OS_IPHONE)
    int                 tex_width;
    int                 tex_height;
#endif
};

/*
** Interface implementation
*/
class BSGL_Impl : public BSGL {
public:
    virtual void    CALL Release();

    virtual bool    CALL System_Initiate();
    virtual void    CALL System_Shutdown();
    virtual bool    CALL System_Start();

    virtual char*   CALL System_GetErrorMessage();

    virtual void    CALL System_SetStateBool(bsglBoolState state, bool value);
    virtual void    CALL System_SetStateFunc(bsglFuncState state, bsglCallback value);
    virtual void    CALL System_SetStateInt(bsglIntState state, int value);
    virtual int     CALL System_GetStateInt(bsglIntState state);
    virtual void    CALL System_SetStateString(bsglStringState state, char const* value);

    virtual void    CALL System_Log(char const* format, ...);

    virtual void    CALL Config_SetInt(char const* section, char const* option, int value);
    virtual int     CALL Config_GetInt(char const* section, char const* option, int def_val);
    virtual void    CALL Config_SetFloat(char const* section, char const* option, float value);
    virtual float   CALL Config_GetFloat(char const* section, char const* option, float def_val);
    virtual void    CALL Config_SetString(char const* section, char const* option, char const* value);
    virtual char*   CALL Config_GetString(char const* section, char const* option, char const* def_val);

    virtual void    CALL Random_Seed(int seed=0);
    virtual int     CALL Random_Int(int min, int max);
    virtual float   CALL Random_Float(float min, float max);

    virtual float   CALL Timer_GetTime();
    virtual float   CALL Timer_GetDelta();
    virtual int     CALL Timer_GetFPS();

    virtual bool        CALL Gfx_BeginScene();
    virtual void        CALL Gfx_EndScene();
    virtual void        CALL Gfx_Clear(DWORD color);
    virtual void        CALL Gfx_RenderTriple(const bsglTriple* triple);
    virtual void        CALL Gfx_RenderQuad(const bsglQuad* quad);
    virtual bsglVertex* CALL Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend, int* max_prim);
    virtual void        CALL Gfx_FinishBatch(int nprim);
    virtual void        CALL Gfx_SetClipping(int x=0, int y=0, int w=0, int h=0);
    virtual void        CALL Gfx_SetTransform(float x=0, float y=0, float dx=0, float dy=0, float rot=0, float hscale=0, float vscale=0);

    virtual HTEXTURE    CALL Texture_Create(int width, int height);
    virtual HTEXTURE    CALL Texture_Load(char const* filename, DWORD size=0, bool bMipmap=false);
    virtual void        CALL Texture_Free(HTEXTURE tex);
    virtual int         CALL Texture_GetWidth(HTEXTURE tex, bool bOriginal=false);
    virtual int         CALL Texture_GetHeight(HTEXTURE tex, bool bOriginal=false);
    virtual DWORD*      CALL Texture_CreateData(int width, int height);
    virtual DWORD*      CALL Texture_LoadData(HTEXTURE tex);
    virtual void        CALL Texture_FreeData(DWORD* data);
    virtual void        CALL Texture_Update(HTEXTURE tex, DWORD* data, int x=0, int y=0, int width=0, int height=0);

    virtual void        CALL Control_GetState();
    virtual bool        CALL Control_IsDown(int key);
    virtual bool        CALL Control_IsPassing(int key);
    virtual bool        CALL Control_IsUp(int key);
    virtual int         CALL Control_GetMouseX();
    virtual int         CALL Control_GetMouseY();

    static BSGL_Impl*   _Interface_Get();

    void                _LoadConfig(char const* filename);
    void                _PostError(char const* error, ...);

private:
    char szTitle[256];
    char szError[256];
    bool bWindowed;
    int nScreenWidth;
    int nScreenHeight;
    int nScreenBPP;
    bool bActive;
#if !defined(CC_TARGET_OS_IPHONE)
    SDL_Surface* pSurface;
#endif
    char szLogFile[256];
    char szCfgFile[256];
    char szCfgString[256];

    TextureList*    textures;
    GLubyte*        indexes;
    bsglVertex*     VertArray;

    int         nPrim;
    int         CurPrimType;
    int         CurBlendMode;
    HTEXTURE    CurTexture;
    int         nPolyMode;

    int         nLFPS;          // Number of logic frames per second
    int         nRFPS;          // Number of render frames per second
    FrameFunc   fpLogicFunc;    // Logic process callback function
    FrameFunc   fpRenderFunc;   // Render process callback function

    float   fTime;
    float   fDeltaTime;
    int     nFPS;
#if defined(CC_TARGET_OS_IPHONE)
    float   fStartTime;
#endif

    unsigned int*       _key_buf;
    int                 _mouse_x;
    int                 _mouse_y;

    bool _GfxInit();
    void _GfxDone();

    void _render_batch(bool bEndScene=false);
    void _SetBlendMode(int blend);

private:
    BSGL_Impl();
};

extern BSGL_Impl* pBSGL;

#endif//BSGL_IMPL_H

