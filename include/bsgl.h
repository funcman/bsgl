/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** System layer API
*/

#ifndef BSGL_H
#define BSGL_H

#define BSGL_VERSION 0x010

#define EXPORT
#define CALL// __stdcall

/*
** Common data types
*/
#ifndef DWORD
typedef unsigned long       DWORD;
typedef unsigned short      WORD;
typedef unsigned char       BYTE;
#endif

/*
** Common math constants
*/
#ifndef M_PI
#define M_PI    3.14159265358979323846f
#define M_PI_2  1.57079632679489661923f
#define M_PI_4  0.785398163397448309616f
#define M_1_PI  0.318309886183790671538f
#define M_2_PI  0.636619772367581343076f
#endif

/*
** Handle types
*/
typedef DWORD HTEXTURE;
//typedef DWORD HTARGET;
typedef DWORD HEFFECT;
typedef DWORD HMUSIC;
typedef DWORD HSTREAM;
typedef DWORD HCHANNEL;

/*
** Hardware color macros
*/
#define RGBA(r,g,b,a)   ((DWORD(a)<<24) + (DWORD(b)<<16) + (DWORD(g)<<8) + DWORD(r))
#define GETR(col)       ((col) & 0xFF)
#define GETG(col)       (((col)>>8) & 0xFF)
#define GETB(col)       (((col)>>16) & 0xFF)
#define GETA(col)       ((col)>>24)
#define SETR(col,r)     (((col) & 0xFFFFFF00) + DWORD(r))
#define SETG(col,g)     (((col) & 0xFFFF00FF) + (DWORD(g)<<8))
#define SETB(col,b)     (((col) & 0xFF00FFFF) + (DWORD(b)<<16))
#define SETA(col,a)     (((col) & 0x00FFFFFF) + (DWORD(a)<<24))

/*
** Blending constants
*/
#define BLEND_COLORADD      1
#define BLEND_COLORMUL      0
#define BLEND_ALPHABLEND    2
#define BLEND_ALPHAADD      0
#define BLEND_ZWRITE        4
#define BLEND_NOZWRITE      0
#define BLEND_DEFAULT       (BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_NOZWRITE)
#define BLEND_DEFAULT_Z     (BLEND_COLORMUL | BLEND_ALPHABLEND | BLEND_ZWRITE)

/*
** Input flags
*/
#define INP_MOUSEL      0
#define INP_MOUSER      1

#define INP_A           65
#define INP_B           66
#define INP_C           67
#define INP_D           68
#define INP_E           69
#define INP_F           70
#define INP_G           71
#define INP_H           72
#define INP_I           73
#define INP_J           74
#define INP_K           75
#define INP_L           76
#define INP_M           77
#define INP_N           78
#define INP_O           79
#define INP_P           80
#define INP_Q           81
#define INP_R           82
#define INP_S           83
#define INP_T           84
#define INP_U           85
#define INP_V           86
#define INP_W           87
#define INP_X           88
#define INP_Y           89
#define INP_Z           90


#define INP_1           101
#define INP_2           102
#define INP_3           103
#define INP_4           104
#define INP_5           105
#define INP_6           106
#define INP_7           107
#define INP_8           108
#define INP_9           109
#define INP_0           110

#define INP_F1          121
#define INP_F2          122
#define INP_F3          123
#define INP_F4          124
#define INP_F5          125
#define INP_F6          126
#define INP_F7          127
#define INP_F8          128
#define INP_F9          129
#define INP_F10         130
#define INP_F11         131
#define INP_F12         132

#define INP_ESC         160
#define INP_TAB         161
#define INP_CAPSLOCK    162
#define INP_SHIFTL      163
#define INP_SHIFTR      164
#define INP_CTRLL       165
#define INP_CTRLR       166
#define INP_ALTL        167
#define INP_ALTR        168
#define INP_SPACE       169
#define INP_ENTER       170
#define INP_DEL         171

#define INP_UP          181
#define INP_DOWN        182
#define INP_LEFT        183
#define INP_RIGHT       184

#define INP_HOME        191
#define INP_END         192
#define INP_PGUP        193
#define INP_PGDN        194

#define INP_TOUCH       255

/*
** System state constants
*/
enum bsglBoolState {
    BSGL_WINDOWED = 1
};

enum bsglFuncState {
    BSGL_LOGICFUNC   = 1,
    BSGL_RENDERFUNC  = 2
};

enum bsglIntState
{
    BSGL_SCREENWIDTH     = 1,
    BSGL_SCREENHEIGHT    = 2,
    BSGL_SCREENBPP       = 3,
    BSGL_NUMOFLFPS       = 4,
    BSGL_NUMOFRFPS       = 5,
    BSGL_POLYMODE        = 6
};

enum bsglStringState {
    BSGL_TITLE   = 1,
    BSGL_CFGFILE = 2,
    BSGL_LOGFILE = 3
};

/*
** Callback protoype used by BSGL
*/
typedef bool (*bsglCallback)();

/*
** Primitive type constants
*/
#define BSGLPRIM_LINES   2
#define BSGLPRIM_TRIPLES 3
#define BSGLPRIM_QUADS   4

/*
** Vertex structure
*/
struct bsglVertex {
    /* Texture coordinates */
    float           tx;
    float           ty;
    /* Color */
    union {
        struct {
            unsigned char   red;
            unsigned char   green;
            unsigned char   blue;
            unsigned char   alpha;
        };
        unsigned int color;
    };
    /* Position */
    float           x;
    float           y;
    float           z;
};

/*
** Triple structure
*/
struct bsglTriple {
    bsglVertex  v[3];
    HTEXTURE    tex;
    int         blend;
};

/*
** Quad structure
*/
struct bsglQuad {
    bsglVertex  v[4];
    HTEXTURE    tex;
    int         blend;
};

/*
** Interface class
*/
class BSGL {
public:
    virtual void        CALL Release() = 0;

    virtual bool        CALL System_Initiate() = 0;
    virtual void        CALL System_Shutdown() = 0;
    virtual bool        CALL System_Start() = 0;

    virtual char*       CALL System_GetErrorMessage() = 0;

    virtual void        CALL System_SetStateBool(bsglBoolState state, bool value) = 0;
    virtual void        CALL System_SetStateFunc(bsglFuncState state, bsglCallback value) = 0;
    virtual void        CALL System_SetStateInt(bsglIntState state, int value) = 0;
    virtual int         CALL System_GetStateInt(bsglIntState state) = 0;
    virtual void        CALL System_SetStateString(bsglStringState state, char const* value) = 0;

    virtual void        CALL System_Log(char const *format, ...) = 0;

    virtual void        CALL Config_SetInt(char const* section, char const* option, int value) = 0;
    virtual int         CALL Config_GetInt(char const* section, char const* option, int def_val) = 0;
    virtual void        CALL Config_SetFloat(char const* section, char const* option, float value) = 0;
    virtual float       CALL Config_GetFloat(char const* section, char const* option, float def_val) = 0;
    virtual void        CALL Config_SetString(char const* section, char const* option, char const* value) = 0;
    virtual char*       CALL Config_GetString(char const* section, char const* option, char const* def_val) = 0;

    virtual void        CALL Random_Seed(int seed=0) = 0;
    virtual int         CALL Random_Int(int min, int max) = 0;
    virtual float       CALL Random_Float(float min, float max) = 0;

    virtual float       CALL Timer_GetTime() = 0;
    virtual float       CALL Timer_GetDelta() = 0;
    virtual int         CALL Timer_GetFPS() = 0;

    virtual bool        CALL Gfx_BeginScene() = 0;
    virtual void        CALL Gfx_EndScene() = 0;
    virtual void        CALL Gfx_Clear(DWORD color) = 0;
    virtual void        CALL Gfx_RenderTriple(const bsglTriple* triple) = 0;
    virtual void        CALL Gfx_RenderQuad(const bsglQuad* quad) = 0;
    virtual bsglVertex* CALL Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend, int* max_prim) = 0;
    virtual void        CALL Gfx_FinishBatch(int nprim) = 0;
    virtual void        CALL Gfx_SetClipping(int x=0, int y=0, int w=0, int h=0) = 0;
    virtual void        CALL Gfx_SetTransform(float x=0, float y=0, float dx=0, float dy=0, float rot=0, float hscale=0, float vscale=0) = 0;

    virtual HTEXTURE    CALL Texture_Create(int width, int height) = 0;
    virtual HTEXTURE    CALL Texture_Load(char const* filename, DWORD size=0, bool bMipmap=false) = 0;
    virtual void        CALL Texture_Free(HTEXTURE tex) = 0;
    virtual int         CALL Texture_GetWidth(HTEXTURE tex, bool bOriginal=false) = 0;
    virtual int         CALL Texture_GetHeight(HTEXTURE tex, bool bOriginal=false) = 0;
    virtual DWORD*      CALL Texture_CreateData(int width, int height) = 0;
    virtual DWORD*      CALL Texture_LoadData(HTEXTURE tex) = 0;
    virtual void        CALL Texture_FreeData(DWORD* data) = 0;
    virtual void        CALL Texture_Update(HTEXTURE tex, DWORD* data, int x=0, int y=0, int width=0, int height=0) = 0;

    virtual void        CALL Control_GetState() = 0;
    virtual bool        CALL Control_IsDown(int key) = 0;
    virtual bool        CALL Control_IsPassing(int key) = 0;
    virtual bool        CALL Control_IsUp(int key) = 0;
    virtual int         CALL Control_GetMouseX() = 0;
    virtual int         CALL Control_GetMouseY() = 0;
};

extern "C" { EXPORT BSGL* CALL bsglCreate(int ver); }

#endif//BSGL_H

