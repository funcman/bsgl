/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** Core functions implementation: graphics
*/

#include "bsgl_impl.h"

#include <spng.h>
#include <turbojpeg.h>

void _InitOGL();
void _Resize(int, int);

struct _Bitmap {
    int             ow;
    int             oh;
    int             tw;
    int             th;
    unsigned char*  data;
};

struct _Bitmap* _LoadBMP(char const* filename);
struct _Bitmap* _LoadPNG(char const* filename);
struct _Bitmap* _LoadJPG(char const* filename);
void            _FreeBitmap(struct _Bitmap* bitmap);

int powerOfTwo(int num);

// case-insensitive file extension compare (".png" vs ".PNG")
static bool _MatchExt(char const* filename, char const* ext) {
    char const* dot = strrchr(filename, '.');
    if (!dot) {
        return false;
    }
    while (*dot && *ext) {
        char a = *dot++;
        char b = *ext++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) {
            return false;
        }
    }
    return *dot == *ext;
}

void CALL BSGL_Impl::Gfx_Clear(DWORD color) {
    // glClearColor expects normalized 0..1 floats; GETR/... yield 0..255
    glClearColor((GLfloat)GETR(color)/255.0f, (GLfloat)GETG(color)/255.0f,
                 (GLfloat)GETB(color)/255.0f, (GLfloat)GETA(color)/255.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

bool CALL BSGL_Impl::Gfx_BeginScene() {
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
    return true;
}

void CALL BSGL_Impl::Gfx_EndScene() {
    _render_batch(true);
}

void CALL BSGL_Impl::Gfx_RenderTriple(const bsglTriple* triple) {
    if ((CurPrimType != BSGLPRIM_TRIPLES)
    ||  (nPrim >= VERTEX_BUFFER_SIZE/BSGLPRIM_TRIPLES)
    ||  (CurTexture != triple->tex)
    ||  (CurBlendMode != triple->blend)) {
        _render_batch();

        CurPrimType = BSGLPRIM_TRIPLES;
        if (CurBlendMode != triple->blend) {
            _SetBlendMode(triple->blend);
        }
        if (CurTexture != triple->tex) {
            if (triple->tex) {
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)triple->tex);
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            CurTexture = triple->tex;
        }
    }
    memcpy(&VertArray[nPrim*BSGLPRIM_TRIPLES], triple->v, sizeof(bsglVertex)*BSGLPRIM_TRIPLES);
    ++nPrim;
}

void CALL BSGL_Impl::Gfx_RenderQuad(const bsglQuad* quad) {
    if ((CurPrimType != BSGLPRIM_QUADS)
    ||  (nPrim >= VERTEX_BUFFER_SIZE/BSGLPRIM_QUADS)
    ||  (CurTexture != quad->tex)
    ||  (CurBlendMode != quad->blend)) {
        _render_batch();

        CurPrimType = BSGLPRIM_QUADS;
        if (CurBlendMode != quad->blend) {
            _SetBlendMode(quad->blend);
        }
        if (CurTexture != quad->tex) {
            if (quad->tex) {
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)quad->tex);
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            CurTexture = quad->tex;
        }
    }
    memcpy(&VertArray[nPrim*BSGLPRIM_QUADS], quad->v, sizeof(bsglVertex)*BSGLPRIM_QUADS);
    ++nPrim;
}

bsglVertex* CALL BSGL_Impl::Gfx_StartBatch(int prim_type, HTEXTURE tex, int blend, int* max_prim) {
    if (VertArray) {
        _render_batch();

        CurPrimType = prim_type;
        if (CurBlendMode != blend) {
            _SetBlendMode(blend);
        }
        if (CurTexture != tex) {
            glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
            CurTexture = tex;
        }

        *max_prim = VERTEX_BUFFER_SIZE / prim_type;
        return VertArray;
    } else {
        return nullptr;
    }
}

void CALL BSGL_Impl::Gfx_FinishBatch(int nprim) {
    nPrim = nprim;
}

HTEXTURE CALL BSGL_Impl::Texture_Create(int width, int height) {
    GLuint* texture = new GLuint;
    int ow = width;
    int oh = height;
    int tw = powerOfTwo(width);
    int th = powerOfTwo(height);
    *texture = 0;

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th,
            0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextureList* texItem = new TextureList;
    texItem->tex = (HTEXTURE)texture;
    texItem->width = ow;
    texItem->height = oh;
    texItem->next = textures;
    textures = texItem;

    return (HTEXTURE)texture;
}

HTEXTURE CALL BSGL_Impl::Texture_Load(const char* filename, DWORD size, bool bMipmap) {
    GLuint* texture = new GLuint;
    struct _Bitmap* texture_image = nullptr;
    *texture = 0;

    // the decoder is chosen by the file name suffix; anything that is
    // not .png/.jpg/.jpeg is treated as an uncompressed 32-bit BMP
    if (_MatchExt(filename, ".png")) {
        texture_image = _LoadPNG(filename);
    } else if (_MatchExt(filename, ".jpg") || _MatchExt(filename, ".jpeg")) {
        texture_image = _LoadJPG(filename);
    } else {
        texture_image = _LoadBMP(filename);
    }

    if (texture_image) {
        glGenTextures(1, texture);
        glBindTexture(GL_TEXTURE_2D, *texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_image->tw,
                texture_image->th, 0, GL_BGRA_EXT,
                GL_UNSIGNED_BYTE, texture_image->data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!texture_image) {
        _PostError("Can't load the texture file \"%s\".", filename);
        return (HTEXTURE)0;
    }

    TextureList* texItem = new TextureList;
    texItem->tex = (HTEXTURE)texture;
    texItem->width = texture_image->ow;
    texItem->height = texture_image->oh;
    texItem->next = textures;
    textures = texItem;

    _FreeBitmap(texture_image);

    return (HTEXTURE)texture;
}

void CALL BSGL_Impl::Texture_Free(HTEXTURE tex) {
    TextureList* texItem = textures;
    TextureList* texPrev = nullptr;

    while (texItem) {
        if (texItem->tex == tex) {
            if (texPrev) {
                texPrev->next = texItem->next;
            } else {
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

    if (bOriginal) {
        while (texItem) {
            if (texItem->tex == tex) {
                return texItem->width;
            }
            texItem = texItem->next;
        }
        return 0;
    }

    GLint width;
    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    return (int)width;
}

int CALL BSGL_Impl::Texture_GetHeight(HTEXTURE tex, bool bOriginal) {
    TextureList* texItem = textures;

    if (bOriginal) {
        while (texItem) {
            if (texItem->tex == tex) {
                return texItem->height;
            }
            texItem = texItem->next;
        }
        return 0;
    }

    GLint height;
    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    return (int)height;
}

DWORD* CALL BSGL_Impl::Texture_CreateData(int width, int height) {
    return new DWORD[width*height];
}

DWORD* CALL BSGL_Impl::Texture_LoadData(HTEXTURE tex) {
    int width;
    int height;
    DWORD* data;

    glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    data = new DWORD[width*height];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);

    return data;
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

    if (0 == width) {
        _x = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &_w);
    } else {
        _x = x;
        _w = width;
    }

    if (0 == height) {
        _y = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &_h);
    } else {
        _y = y;
        _h = height;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, _w, _h, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);
}

void CALL BSGL_Impl::Gfx_SetClipping(int x, int y, int w, int h) {//remember to test w and h are negative
    if (0 == w || 0== h) {
        x = 0;
        y = 0;
        w = nScreenWidth;
        h = nScreenHeight;
    } else {
        if (x < 0) { w+=x; x=0; }
        if (y < 0) { h+=y; y=0; }
        if (x+w > nScreenWidth) w = nScreenWidth - x;
        if (y+h > nScreenHeight) h = nScreenHeight - y;
    }

    _render_batch();
    glViewport(x, nScreenHeight-y-h, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
    glOrtho((GLdouble)x, (GLdouble)(x+w), (GLdouble)y, (GLdouble)(y+h), -1.0, 1.0);
}

// fuck! the function is difficult to write.
void CALL BSGL_Impl::Gfx_SetTransform(float x, float y, float dx, float dy, float rot, float hscale, float vscale) {
    _render_batch();
    glMatrixMode(GL_MODELVIEW);
    if (0.0f == hscale || 0.0f == vscale) {
        glLoadIdentity();
    } else {
        glLoadIdentity();
        glTranslatef(x+dx, y+dy, 0.0f);
        glRotatef(rot, 0.0f, 0.0f, -1.0f);
        glScalef(hscale, vscale, 1.0f);
        glTranslatef(-x, -y, 0.0f);
    }
}

bool BSGL_Impl::_GfxInit() {
    GLushort* iptr = indexes = new GLushort[VERTEX_BUFFER_SIZE*6/4];
    GLushort n = 0;
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
    if (nullptr == VertArray) {
        _PostError("Can't create vertex buffer");
        return false;
    }

    textures = nullptr;

    nPrim = 0;
    CurPrimType = BSGLPRIM_QUADS;
    CurBlendMode = BLEND_DEFAULT;
    CurTexture = 0;

    return true;
}

void BSGL_Impl::_GfxDone() {
    delete[] VertArray;
    VertArray = nullptr;
    delete[] indexes;
    indexes = nullptr;
    while (textures) {
        Texture_Free(textures->tex);
    }
}

void BSGL_Impl::_render_batch(bool bEndScene) {
    if (nPrim != 0) {
        switch(CurPrimType) {
        case BSGLPRIM_LINES:
            break;
        case BSGLPRIM_TRIPLES:
        case BSGLPRIM_QUADS: {
            // Re-bind the batch texture: never trust the implicit GL
            // binding, other code paths (Texture_LoadData/Update/...)
            // bind textures behind the batch renderer's back.
            glBindTexture(GL_TEXTURE_2D,
                          CurTexture ? *(GLuint*)CurTexture : 0);
            GLsizei stride = sizeof(bsglVertex);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glVertexPointer(3, GL_FLOAT, stride, &VertArray[0].x);
            glColorPointer(4, GL_UNSIGNED_BYTE, stride, &VertArray[0].red);
            glTexCoordPointer(2, GL_FLOAT, stride, &VertArray[0].tx);
            if (CurPrimType == BSGLPRIM_TRIPLES) {
                glDrawArrays(GL_TRIANGLES, 0, nPrim*BSGLPRIM_TRIPLES);
            } else {
                glDrawElements(GL_TRIANGLES, nPrim*6, GL_UNSIGNED_SHORT, indexes);
            }
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            break;
        }
        default:
            break;
        }
        nPrim = 0;
    }
}

void BSGL_Impl::_SetBlendMode(int blend) {
    if ((blend & BLEND_ALPHABLEND) != (CurBlendMode & BLEND_ALPHABLEND)) {
        if (blend & BLEND_ALPHABLEND) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
    }

    if ((blend & BLEND_ZWRITE) != (CurBlendMode & BLEND_ZWRITE)) {
        if (blend & BLEND_ZWRITE) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    if ((blend & BLEND_COLORADD) != (CurBlendMode & BLEND_COLORADD)) {
        if (blend & BLEND_COLORADD) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
        } else {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    CurBlendMode = blend;
}

void _InitOGL() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void _Resize(int width, int height) {
    if (height == 0) {
        height = 1;
    }
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
    glOrtho(0.0, (GLdouble)width, 0.0, (GLdouble)height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int powerOfTwo(int num) {
    int r = 2;
    for (;r<num;) {
        r <<= 1;
    }
    return r;
}

// build the power-of-two padded texture image from a decoded
// top-down pixel buffer; set swapRB when the source is RGBA
static struct _Bitmap* _CreateImageBGRA(unsigned char const* pixels, int w, int h, bool swapRB) {
    struct _Bitmap* bitmap = (struct _Bitmap*)malloc(sizeof(struct _Bitmap));
    bitmap->ow = w;
    bitmap->oh = h;
    bitmap->tw = powerOfTwo(w);
    bitmap->th = powerOfTwo(h);
    bitmap->data = (unsigned char*)malloc(bitmap->tw * bitmap->th * sizeof(unsigned int));
    memset(bitmap->data, 0, bitmap->tw * bitmap->th * sizeof(unsigned int));
    for (int y = 0; y < h; ++y) {
        unsigned char* dst = bitmap->data + y * bitmap->tw * sizeof(unsigned int);
        unsigned char const* src = pixels + y * w * sizeof(unsigned int);
        if (swapRB) {
            for (int x = 0; x < w; ++x) {
                dst[x*4+0] = src[x*4+2];
                dst[x*4+1] = src[x*4+1];
                dst[x*4+2] = src[x*4+0];
                dst[x*4+3] = src[x*4+3];
            }
        } else {
            memcpy(dst, src, w * sizeof(unsigned int));
        }
    }
    return bitmap;
}

void _FreeBitmap(struct _Bitmap* bitmap) {
    free(bitmap->data);
}

struct _Bitmap* _LoadBMP(char const* filename) {
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
    FILE* file = fopen(filename, "rb");
    struct _Bitmap* bitmap;
    if (!file) {
        return nullptr;
    }
    bitmap = (struct _Bitmap*)malloc(sizeof(struct _Bitmap));
    fread(&header, sizeof(header), 1, file);
    fread(&info, sizeof(info), 1, file);
    bitmap->ow = info.width;
    bitmap->oh = info.height;
    bitmap->tw = powerOfTwo(bitmap->ow);
    bitmap->th = powerOfTwo(bitmap->oh);
    bitmap->data = (unsigned char*)malloc((bitmap->tw)*(bitmap->th)*sizeof(unsigned int));
    memset(bitmap->data, 0, bitmap->tw*bitmap->th*sizeof(unsigned int));
    fseek(file, header.offset, SEEK_SET);
    for(int h=bitmap->oh-1; h>=0; --h) {
        fread((bitmap->data)+h*bitmap->tw*sizeof(unsigned int), sizeof(unsigned int)*bitmap->ow, 1, file);
    }
    fclose(file);
    return bitmap;
}

struct _Bitmap* _LoadPNG(char const* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* png = (unsigned char*)malloc(size);
    if (!png || fread(png, 1, size, file) != (size_t)size) {
        free(png);
        fclose(file);
        return nullptr;
    }
    fclose(file);

    struct _Bitmap* bitmap = nullptr;
    spng_ctx* ctx = spng_ctx_new(0);
    if (ctx && spng_set_png_buffer(ctx, png, size) == 0) {
        struct spng_ihdr ihdr;
        size_t out_size = 0;
        if (spng_get_ihdr(ctx, &ihdr) == 0
        &&  spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size) == 0) {
            unsigned char* pixels = (unsigned char*)malloc(out_size);
            if (pixels
            &&  spng_decode_image(ctx, pixels, out_size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS) == 0) {
                bitmap = _CreateImageBGRA(pixels, (int)ihdr.width, (int)ihdr.height, true);
            }
            free(pixels);
        }
    }
    if (ctx) {
        spng_ctx_free(ctx);
    }
    free(png);
    return bitmap;
}

struct _Bitmap* _LoadJPG(char const* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* jpg = (unsigned char*)malloc(size);
    if (!jpg || fread(jpg, 1, size, file) != (size_t)size) {
        free(jpg);
        fclose(file);
        return nullptr;
    }
    fclose(file);

    struct _Bitmap* bitmap = nullptr;
    tjhandle tj = tjInitDecompress();
    if (tj) {
        int w = 0, h = 0, subsamp = 0, colorspace = 0;
        if (tjDecompressHeader3(tj, jpg, size, &w, &h, &subsamp, &colorspace) == 0) {
            unsigned char* pixels = (unsigned char*)malloc((size_t)w * h * 4);
            if (pixels
            &&  tjDecompress2(tj, jpg, size, pixels, w, 0, h, TJPF_BGRA, 0) == 0) {
                bitmap = _CreateImageBGRA(pixels, w, h, false);
            }
            free(pixels);
        }
        tjDestroy(tj);
    }
    free(jpg);
    return bitmap;
}
