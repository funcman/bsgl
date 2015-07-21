/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: graphics
*/

#include "bsgl_impl.h"

static void _InitOGL();
static void _Resize(int, int);

struct _BMP {
    int             ow;
    int             oh;
    int             tw;
    int             th;
    unsigned char*  data;
};

struct _BMP*    _LoadBMP(char const* filename);
void            _FreeBMP(struct _BMP* bmp);

int next_p2(int num);

void CALL BSGL_Impl::Gfx_Clear(DWORD color) {
    glClearColor((GLfloat)GETR(color), (GLfloat)GETG(color),
                 (GLfloat)GETB(color), (GLfloat)GETA(color));
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

bool CALL BSGL_Impl::Gfx_BeginScene() {
#if !defined(CC_TARGET_OS_IPHONE)
    switch( nPolyMode ) {
    default:
    case 0:
        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    case 1:
        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case 2:
        glDisable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    }
#endif
    return true;
}

void CALL BSGL_Impl::Gfx_EndScene() {
    _render_batch(true);
}

void CALL BSGL_Impl::Gfx_RenderTriple(const bsglTriple* triple) {
    if( (CurPrimType != BSGLPRIM_TRIPLES)
    ||  (nPrim >= VERTEX_BUFFER_SIZE/BSGLPRIM_TRIPLES)
    ||  (CurTexture != triple->tex)
    ||  (CurBlendMode != triple->blend) ) {
        _render_batch();

        CurPrimType = BSGLPRIM_TRIPLES;
        if( CurBlendMode != triple->blend ) {
            _SetBlendMode(triple->blend);
        }
        if( CurTexture != triple->tex ) {
            if( triple->tex )
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)triple->tex);
            else
                glBindTexture(GL_TEXTURE_2D, 0);
            CurTexture = triple->tex;
        }
    }
    memcpy(&VertArray[nPrim*BSGLPRIM_TRIPLES], triple->v, sizeof(bsglVertex)*BSGLPRIM_TRIPLES);
    ++nPrim;
}

void CALL BSGL_Impl::Gfx_RenderQuad(const bsglQuad* quad) {
    if( (CurPrimType != BSGLPRIM_QUADS)
    ||  (nPrim >= VERTEX_BUFFER_SIZE/BSGLPRIM_QUADS)
    ||  (CurTexture != quad->tex)
    ||  (CurBlendMode != quad->blend) ) {
        _render_batch();

        CurPrimType = BSGLPRIM_QUADS;
        if( CurBlendMode != quad->blend ) {
            _SetBlendMode(quad->blend);
        }
        if( CurTexture != quad->tex ) {
            if( quad->tex )
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)quad->tex);
            else
                glBindTexture(GL_TEXTURE_2D, 0);
            CurTexture = quad->tex;
        }
    }
    memcpy(&VertArray[nPrim*BSGLPRIM_QUADS], quad->v, sizeof(bsglVertex)*BSGLPRIM_QUADS);
    ++nPrim;
}

bsglVertex* CALL BSGL_Impl::Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend, int* max_prim) {
    if( VertArray ) {
        _render_batch();

        CurPrimType = prim_type;
        if( CurBlendMode != blend ) {
            _SetBlendMode(blend);
        }
        if( CurTexture != tex ) {
            glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
            CurTexture = tex;
        }

        *max_prim = VERTEX_BUFFER_SIZE / prim_type;
        return VertArray;
    }else {
        return 0;
    }
}

void CALL BSGL_Impl::Gfx_FinishBatch(int nprim) {
    nPrim = nprim;
}

// OLD
/*
HTEXTURE CALL BSGL_Impl::Texture_Create(int width, int height) {
    GLuint* texture = new GLuint;

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return (HTEXTURE)texture;
}
*/

HTEXTURE CALL BSGL_Impl::Texture_Create(int width, int height) {
    GLuint* texture = new GLuint;
    int ow = width;
    int oh = height;
    int tw = next_p2(width);
    int th = next_p2(height);
    *texture = 0;

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextureList* texItem = new TextureList;
    texItem->tex = (HTEXTURE)texture;
    texItem->width = ow;
    texItem->height = oh;
    texItem->next = textures;
#if defined(CC_TARGET_OS_IPHONE)
    texItem->tex_width = tw;
    texItem->tex_height = th;
#endif
    textures = texItem;

    return (HTEXTURE)texture;
}

HTEXTURE CALL BSGL_Impl::Texture_Load(const char* filename, DWORD size, bool bMipmap) {
    GLuint* texture = new GLuint;
    struct _BMP* texture_image = 0;
    *texture = 0;
    if( (texture_image=_LoadBMP(filename)) ) {
        glGenTextures(1, texture);
        glBindTexture(GL_TEXTURE_2D, *texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_image->tw,
                texture_image->th, 0, GL_BGRA_EXT,
                GL_UNSIGNED_BYTE, texture_image->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if( !texture_image ) {
        _PostError("Can't load the texture file \"%s\".", filename);
        return (HTEXTURE)0;
    }

    TextureList* texItem = new TextureList;
    texItem->tex = (HTEXTURE)texture;
    texItem->width = texture_image->ow;
    texItem->height = texture_image->oh;
    texItem->next = textures;
#if defined(CC_TARGET_OS_IPHONE)
    texItem->tex_width = texture_image->tw;
    texItem->tex_height = texture_image->th;
#endif
    textures = texItem;

    _FreeBMP(texture_image);

    return (HTEXTURE)texture;
}

void CALL BSGL_Impl::Texture_Free(HTEXTURE tex) {
    TextureList* texItem = textures;
    TextureList* texPrev = 0;

    while( texItem ) {
        if( texItem->tex == tex ) {
            if( texPrev ) {
                texPrev->next = texItem->next;
            }else {
                textures = texItem->next;
            }
            delete texItem;
            break;
        }
        texPrev = texItem;
        texItem = texItem->next;
    }

    glDeleteTextures(1, (GLuint*)tex);
    delete (GLuint*)tex;
}

int CALL BSGL_Impl::Texture_GetWidth(HTEXTURE tex, bool bOriginal) {
    TextureList* texItem = textures;

    if( bOriginal ) {
        while( texItem ) {
            if( texItem->tex == tex ) {
                return texItem->width;
            }
            texItem = texItem->next;
        }
        return 0;
    }

#if !defined(CC_TARGET_OS_IPHONE)
    GLint width;
    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    return (int)width;
#else
    while( texItem ) {
        if( texItem->tex == tex ) {
            return texItem->tex_width;
        }
        texItem = texItem->next;
    }
    return 0;
#endif
}

int CALL BSGL_Impl::Texture_GetHeight(HTEXTURE tex, bool bOriginal) {
    TextureList* texItem = textures;

    if( bOriginal ) {
        while( texItem ) {
            if( texItem->tex == tex ) {
                return texItem->height;
            }
            texItem = texItem->next;
        }
        return 0;
    }

#if !defined(CC_TARGET_OS_IPHONE)
    GLint height;
    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    return (int)height;
#else
    while( texItem ) {
        if( texItem->tex == tex ) {
            return texItem->tex_height;
        }
        texItem = texItem->next;
    }
    return 0;
#endif
}

DWORD* CALL BSGL_Impl::Texture_CreateData(int width, int height) {
    DWORD* data;
    //?int tex_w = next_p2(width);
    //?int tex_h = next_p2(height);

    //?data = new DWORD[tex_w*tex_h];
    data = new DWORD[width*height];

    return data;
}

DWORD* CALL BSGL_Impl::Texture_LoadData(HTEXTURE tex) {
#if !defined(CC_TARGET_OS_IPHONE)
    int width;
    int height;
    DWORD* data;

    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    data = new DWORD[width*height];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);

    return data;
#else
    return 0;
#endif
}

void CALL BSGL_Impl::Texture_FreeData(DWORD* data) {
    delete[] data;
}

void CALL BSGL_Impl::Texture_Update(HTEXTURE tex, DWORD* data, int x, int y, int width, int height) {
    int _x;
    int _y;
    int _w;
    int _h;

    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);

    if( 0 == width ) {
        _x = 0;
#if !defined(CC_TARGET_OS_IPHONE)
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &_w);
#else
        TextureList* texItem = textures;
        while( texItem ) {
            if( texItem->tex == tex ) {
                _w = texItem->tex_width;
            }
            texItem = texItem->next;
        }
#endif
    }else {
        _x = x;
        _w = width;
    }

    if( 0 == height ) {
        _y = 0;
#if !defined(CC_TARGET_OS_IPHONE)
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &_h);
#else
        TextureList* texItem = textures;
        while( texItem ) {
            if( texItem->tex == tex ) {
                _h = texItem->tex_height;
            }
            texItem = texItem->next;
        }
#endif
    }else {
        _y = y;
        _h = height;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, _w, _h, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
}

void CALL BSGL_Impl::Gfx_SetClipping(int x, int y, int w, int h) {//remember to test w and h are negative
    if( 0 == w || 0== h ) {
        x = 0;
        y = 0;
        w = nScreenWidth;
        h = nScreenHeight;
    }else {
        if( x < 0 ) { w+=x; x=0; }
        if( y < 0 ) { h+=y; y=0; }
        if( x+w > nScreenWidth ) w = nScreenWidth - x;
        if( y+h > nScreenWidth ) h = nScreenHeight - y;
    }

    _render_batch();
    glViewport(x, nScreenHeight-y-h, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
#if !defined(CC_TARGET_OS_IPHONE)
    gluOrtho2D((GLfloat)x, (GLfloat)(x+w), (GLfloat)y, (GLfloat)(y+h));
#else
    glOrthof((GLfloat)x, (GLfloat)(x+w), (GLfloat)y, (GLfloat)(y+h), -1.0f, 1.0f);
#endif
}

// fuck! the function is difficult to write.
void CALL BSGL_Impl::Gfx_SetTransform(float x, float y, float dx, float dy, float rot, float hscale, float vscale) {
    _render_batch();
    if( 0.0f == hscale || 0.0f == vscale ) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }else {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(x+dx, y+dx, 0.0f);
        glRotatef(rot, 0.0f, 0.0f, -1.0f);
        glScalef(hscale, vscale, 1.0f);
        glTranslatef(-x, -y, 0.0f);
    }
}

bool BSGL_Impl::_GfxInit() {
#if !defined(CC_TARGET_OS_IPHONE)
    const SDL_VideoInfo* video_info;
    int video_flags;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);//remember to test the return value
    video_info = SDL_GetVideoInfo();
    video_flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;
    if( video_info->hw_available ) {
        video_flags |= SDL_HWSURFACE;
    }else {
        video_flags |= SDL_SWSURFACE;
    }
    if( !bWindowed ) {
        video_flags |= SDL_FULLSCREEN;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    pSurface = SDL_SetVideoMode(nScreenWidth, nScreenHeight, nScreenBPP, video_flags);
#endif
    _InitOGL();
    _Resize(nScreenWidth, nScreenHeight);

    GLubyte* iptr = indexes = new GLubyte[VERTEX_BUFFER_SIZE*6/4];
    GLubyte n = 0;
    for( int i=0; i<VERTEX_BUFFER_SIZE/4; ++i ) {
        *iptr++ = n;
        *iptr++ = n+1;
        *iptr++ = n+2;
        *iptr++ = n+2;
        *iptr++ = n+3;
        *iptr++ = n;
        n+=4;
    }

    VertArray = new bsglVertex[VERTEX_BUFFER_SIZE];
    if( 0 == VertArray ) {
        _PostError("Can't create vertex buffer");
        return false;
    }
    textures = 0;

    nPrim = 0;
    CurPrimType = BSGLPRIM_QUADS;
    CurBlendMode = BLEND_DEFAULT;
    CurTexture = 0;

    return true;
}

void BSGL_Impl::_GfxDone() {
    delete[] VertArray;
    delete[] indexes;
    while(textures) {
        Texture_Free(textures->tex);
    }
#if !defined(CC_TARGET_OS_IPHONE)
    SDL_Quit();
#endif
}

void BSGL_Impl::_render_batch(bool bEndScene) {
#if defined(CC_TARGET_OS_IPHONE)
    bsglVertex vv;
#endif
    if( nPrim != 0 ) {
        switch(CurPrimType) {
        case BSGLPRIM_LINES:
            break;
        case BSGLPRIM_TRIPLES:
#if !defined(CC_TARGET_OS_IPHONE)
            glInterleavedArrays(GL_T2F_C4UB_V3F, 0, VertArray);
#else
            glTexCoordPointer(2, GL_FLOAT, sizeof(bsglVertex), VertArray);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(bsglVertex), (unsigned char*)VertArray+((unsigned char*)&(vv.color)-(unsigned char*)&(vv.tx)));
            glVertexPointer(3, GL_FLOAT, sizeof(bsglVertex), (unsigned char*)VertArray+((unsigned char*)&(vv.x)-(unsigned char*)&(vv.tx)));
#endif
            glDrawArrays(GL_TRIANGLES, 0, nPrim*BSGLPRIM_TRIPLES);//remember to test
            break;
        case BSGLPRIM_QUADS:
#if !defined(CC_TARGET_OS_IPHONE)
            glInterleavedArrays(GL_T2F_C4UB_V3F, 0, VertArray);
#else
            glTexCoordPointer(2, GL_FLOAT, sizeof(bsglVertex), VertArray);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(bsglVertex), (unsigned char*)VertArray+((unsigned char*)&(vv.color)-(unsigned char*)&(vv.tx)));
            glVertexPointer(3, GL_FLOAT, sizeof(bsglVertex), (unsigned char*)VertArray+((unsigned char*)&(vv.x)-(unsigned char*)&(vv.tx)));
#endif
            glDrawElements(GL_TRIANGLES, nPrim*6, GL_UNSIGNED_BYTE, indexes);
            break;
        default:
            break;
        }
        nPrim = 0;
    }
}

void BSGL_Impl::_SetBlendMode(int blend) {
    if( ( blend & BLEND_ALPHABLEND ) != ( CurBlendMode & BLEND_ALPHABLEND ) ) {
        if( blend & BLEND_ALPHABLEND ) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
    }

    if( ( blend & BLEND_ZWRITE ) != ( CurBlendMode & BLEND_ZWRITE ) ) {
        if( blend & BLEND_ZWRITE ) {
            glEnable(GL_DEPTH_TEST);
        }else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    if( ( blend & BLEND_COLORADD ) != ( CurBlendMode & BLEND_COLORADD ) ) {
        if( blend & BLEND_COLORADD ) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        }else {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    CurBlendMode = blend;
}

void _InitOGL() {
    //glShadeModel(GL_SMOOTH);
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#if defined(CC_TARGET_OS_IPHONE)
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
}

#if defined(CC_TARGET_OS_IPHONE)
extern int Portrait;
extern float mmGetViewContentScaleFactor();
extern int mmGetViewWidth();
extern int mmGetViewHeight();
#endif

void _Resize(int width, int height) {
    if( height == 0 ) {
        height = 1;
    }
#if !defined(CC_TARGET_OS_IPHONE)
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
#else
    glViewport(0, 0, mmGetViewWidth()*mmGetViewContentScaleFactor(), mmGetViewHeight()*mmGetViewContentScaleFactor());
#endif
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
#if !defined(CC_TARGET_OS_IPHONE)
    gluOrtho2D(0.0, (GLfloat)width, 0.0, (GLfloat)height);
#else
if( Portrait )
    glOrthof(0.0, mmGetViewWidth()*mmGetViewContentScaleFactor(), 0.0, mmGetViewHeight()*mmGetViewContentScaleFactor(), -1.0, 1.0);
else
    glOrthof(0.0, mmGetViewHeight()*mmGetViewContentScaleFactor(), 0.0, mmGetViewWidth()*mmGetViewContentScaleFactor(), -1.0, 1.0);
#endif

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int next_p2(int num) {
    int r = 2;
    for(;r<num;) {
        r<<=1;
    }
    return r;
}

#if defined(CC_TARGET_OS_IPHONE)
extern std::string mmGetFullPath(char const* filename);
#endif

struct _BMP* _LoadBMP(char const* filename) {
#if defined(WIN32)
#pragma pack(push)
#pragma pack(1)
#endif
    struct _BITMAPFILEHEADER {
        unsigned short  bmp_flag;
        unsigned int    size;
        unsigned short  reserved1;
        unsigned short  reserved2;
        unsigned int    offset;
    }
#if defined(__GNUC__)
    __attribute__ ((__packed__)) header;
#else
    header;
#endif
    struct _BITMAPINFOHEADER {
        unsigned int    size;
        int             width;
        int             height;
        unsigned short  plane;
        unsigned short  bitcount;
        unsigned int    compression;
        unsigned int    image_size;
        int             x_pels_per_meter;
        int             y_pels_per_meter;
        unsigned int    color_used;
        unsigned int    color_important;
    }
#if defined(__GNUC__)
    __attribute__ ((__packed__)) info;
#else
    info;
#endif
#if defined(WIN32)
#pragma pack(pop)
#endif
    FILE* fp;
    struct _BMP* bmp;
#if !defined(CC_TARGET_OS_IPHONE)
    fp = fopen(filename, "rb");
#else
    fp = fopen(mmGetFullPath(filename).c_str(), "rb");
#endif
    if( NULL == fp ) {
        return NULL;
    }
    bmp = (struct _BMP*)malloc(sizeof(struct _BMP));
    fread(&header, sizeof(header), 1, fp);
    fread(&info, sizeof(info), 1, fp);
    bmp->ow = info.width;
    bmp->oh = info.height;
    bmp->tw = next_p2(bmp->ow);
    bmp->th = next_p2(bmp->oh);
    bmp->data = (unsigned char*)malloc((bmp->tw)*(bmp->th)*sizeof(unsigned int));
    memset(bmp->data, 0, bmp->tw*bmp->th*sizeof(unsigned int));
    for(int h=bmp->oh-1; h>=0; --h) {
        fread((bmp->data)+h*bmp->tw*sizeof(unsigned int), sizeof(unsigned int)*bmp->ow, 1, fp);
    }
    fclose(fp);
    return bmp;
}

void _FreeBMP(struct _BMP* bmp) {
    free(bmp->data);
}
