/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: graphics
*/

#include "bsgl_impl.h"

void _InitOGL();
void _Resize(int, int);

struct _BMP {
    int             ow;
    int             oh;
    int             tw;
    int             th;
    unsigned char*  data;
};

struct _BMP*    _LoadBMP(char const* filename);
void            _FreeBMP(struct _BMP* bmp);

int powerOfTwo(int num);

static GLchar const* vs[] = {
    "uniform mat4 LProjectionMatrix;"
    "uniform mat4 LModelViewMatrix;"
    "attribute vec3 LVertexPos3D;"
    "attribute vec4 LMultiColor;"
    "attribute vec2 LTexCoord;"
    "varying vec4 multiColor;"
    "varying vec2 texCoord;"
    "void main() {"
    "    multiColor  = LMultiColor;"
    "    texCoord    = LTexCoord;"
    "    gl_Position = LProjectionMatrix * LModelViewMatrix * vec4(LVertexPos3D.x, LVertexPos3D.y, LVertexPos3D.z, 1.0);"
    "}"
};

static GLchar const* fs[] = {
    "precision highp float;"
    "uniform int LIsModulateMode;"
    "uniform sampler2D LTextureUnit;"
    "varying vec4 multiColor;"
    "varying vec2 texCoord;"
    "void main() {"
    "    gl_FragColor = texture2D(LTextureUnit, texCoord);"
    "}"
};

GLint programID;
GLint projectionMatrixLocation;
GLint modelViewMatrixLocation;
GLint vertexPos3DLocation;
GLint multiColorLocation;
GLint texCoordLocation;
GLint isModulateModeLocation;
GLint textureUnitLocation;

static void initGLSL() {
    BSGL_Impl* bsgl = BSGL_Impl::_Interface_Get();
    programID = glCreateProgram();
    // vs
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, vs, 0);
    glCompileShader(vShader);
    GLint vsCompiled = GL_FALSE;
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &vsCompiled);
    if (vsCompiled != GL_TRUE) {
        int maxLen;
        glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &maxLen);
        char* infoLog = new char[maxLen];
        int infoLen;
        glGetShaderInfoLog(vShader, maxLen, &infoLen, infoLog);
        if (infoLen > 0) {
            bsgl->_PostError("%s", infoLog);
        }
        delete[] infoLog;
    }
    glAttachShader(programID, vShader);
    // fs
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, fs, 0);
    glCompileShader(fShader);
    GLint fsCompiled = GL_FALSE;
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &fsCompiled);
    if (fsCompiled != GL_TRUE) {
        int maxLen;
        glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &maxLen);
        char* infoLog = new char[maxLen];
        int infoLen;
        glGetShaderInfoLog(fShader, maxLen, &infoLen, infoLog);
        if (infoLen > 0) {
            bsgl->_PostError("%s", infoLog);
        }
        delete[] infoLog;
    }
    glAttachShader(programID, fShader);
    glLinkProgram(programID);
    GLint success = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        int maxLen;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLen);
        char* infoLog = new char[maxLen];
        int infoLen;
        glGetProgramInfoLog(programID, maxLen, &infoLen, infoLog);
        if (infoLen > 0) {
            bsgl->_PostError("%s", infoLog);
        }
        delete[] infoLog;
    }
    // location
    projectionMatrixLocation    = glGetUniformLocation(programID,   "LProjectionMatrix");
    modelViewMatrixLocation     = glGetUniformLocation(programID,   "LModelViewMatrix");
    vertexPos3DLocation         = glGetAttribLocation(programID,    "LVertexPos3D");
    multiColorLocation          = glGetAttribLocation(programID,    "LMultiColorLocation");
    texCoordLocation            = glGetAttribLocation(programID,    "LTexCoord");
    isModulateModeLocation      = glGetUniformLocation(programID,   "LIsModulateMode");
    textureUnitLocation         = glGetUniformLocation(programID,   "LTextureUnit");
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    glUseProgram(programID);
    bsgl->Release();
}

static void setMatrix(bool isProjectionMatrix, GLfloat const* matrix) {
    if (isProjectionMatrix) {
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, matrix);
    }else {
        glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, matrix);
    }
}

static void enableVertexPointer(bool enable) {
    if (enable) {
        glEnableVertexAttribArray(vertexPos3DLocation);
    }else {
        glDisableVertexAttribArray(vertexPos3DLocation);
    }
}

static void enableColorPointer(bool enable) {
    if (enable) {
        glEnableVertexAttribArray(multiColorLocation);
    }else {
        glDisableVertexAttribArray(multiColorLocation);
    }
}

static void enableTexCoordPointer(bool enable) {
    if (enable) {
        glEnableVertexAttribArray(texCoordLocation);
    }else {
        glDisableVertexAttribArray(texCoordLocation);
    }
}

static void setVertexPointer(GLsizei stride, GLvoid const* data) {
    glVertexAttribPointer(vertexPos3DLocation, 3, GL_FLOAT, GL_FALSE, stride, data);
}

static void setColorPointer(GLsizei stride, GLvoid const* data) {
    glVertexAttribPointer(multiColorLocation, 4, GL_FLOAT, GL_FALSE, stride, data);
}

static void setTexCoordPointer(GLsizei stride, GLvoid const* data) {
    glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, stride, data);
}

static void setTextureUnit(GLuint unit) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, unit);
    glUniform1i(textureUnitLocation, 0);
}

static void setAddMode(bool isAddMode) {
    if (isAddMode) {
        glUniform1i(isModulateModeLocation, 0);
    }else {
        glUniform1i(isModulateModeLocation, 1);
    }
}

static void mxm(GLfloat* m1, GLfloat* m2) {
    GLfloat m[16];
    for (int i=0; i<16; ++i) {
        m[i] = 0.f;
    }
    for (int i=0; i<4; ++i) {
        for (int j=0; j<4; ++j) {
            for (int k=0; k<4; ++k) {
                m[i*4+k] += m1[i*4+j]*m2[j*4+k];
            }
        }
    }
    for (int i=0; i<16; ++i) {
        m2[i] = m[i];
    }
}

static void mi(GLfloat* m) {
    m[0]    = 1.f;  m[1]    = 0.f;      m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;  m[5]    = 1.f;      m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;  m[9]    = 0.f;      m[10]   = 1.f;  m[11]   = 0.f;
    m[12]   = 0.f;  m[13]   = 0.f;      m[14]   = 0.f;  m[15]   = 1.f;
}

static void mt(float dx, float dy, GLfloat* m) {
    m[0]    = 1.f;  m[1]    = 0.f;      m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;  m[5]    = 1.f;      m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;  m[9]    = 0.f;      m[10]   = 1.f;  m[11]   = 0.f;
    m[12]   = dx;   m[13]   = dy;       m[14]   = 0.f;  m[15]   = 1.f;
}

static void mr(float angle, GLfloat* m) {
    m[0]    = cos(3.1415926f*angle/180.f);  m[1]    = -sin(3.1415926f*angle/180.f);     m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = sin(3.1415926f*angle/180.f);  m[5]    = cos(3.1415926f*angle/180.f);      m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;                          m[9]    = 0.f;                              m[10]   = 1.f;  m[11]   = 0.f;
    m[12]   = 0.f;                          m[13]   = 0.f;                              m[14]   = 0.f;  m[15]   = 1.f;
}

static void ms(float hs, float vs, GLfloat* m) {
    m[0]    = hs;   m[1]    = 0.f;      m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;  m[5]    = vs;       m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;  m[9]    = 0.f;      m[10]   = 1.f;  m[11]   = 0.f;
    m[12]   = 0.f;  m[13]   = 0.f;      m[14]   = 0.f;  m[15]   = 1.f;
}

void CALL BSGL_Impl::Gfx_Clear(DWORD color) {
    glClearColor((GLfloat)GETR(color), (GLfloat)GETG(color),
                 (GLfloat)GETB(color), (GLfloat)GETA(color));
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

bool CALL BSGL_Impl::Gfx_BeginScene() {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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
            if( triple->tex ) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)triple->tex);
            }else {
                glBindTexture(GL_TEXTURE_2D, 0);
#else
                setTextureUnit(*(GLuint*)triple->tex);
            }else {
                setTextureUnit(0);
#endif
            }
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
            if( quad->tex ) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
                glBindTexture(GL_TEXTURE_2D, *(GLuint*)quad->tex);
            }else {
                glBindTexture(GL_TEXTURE_2D, 0);
#else
                setTextureUnit(*(GLuint*)quad->tex);
            }else {
                setTextureUnit(0);
#endif
            }
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
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            glBindTexture(GL_TEXTURE_2D, *(GLuint*)tex);
#else
            setTextureUnit(*(GLuint*)tex);
#endif
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
            0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    TextureList* texItem = new TextureList;
    texItem->tex = (HTEXTURE)texture;
    texItem->width = ow;
    texItem->height = oh;
    texItem->next = textures;
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
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
        glBindTexture(GL_TEXTURE_2D, 0);
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
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
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

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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
    return new DWORD[width*height];
}

DWORD* CALL BSGL_Impl::Texture_LoadData(HTEXTURE tex) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
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

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
    glOrtho((GLdouble)x, (GLdouble)(x+w), (GLdouble)y, (GLdouble)(y+h), -1.0, 1.0);
#else
    GLfloat m[16];
    m[0]    = 2.f/(float)w;         m[1]    = 0.f;              m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;                  m[5]    = -2.f/(float)h;    m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;                  m[9]    = 0.f;              m[10]   = -1.f; m[11]   = 0.f;
    m[12]   = -(float)(x+2*w)/w;    m[13]   = (float)(y+2*h)/h; m[14]   = 0.f;  m[15]   = 1.f;
    setMatrix(true, m);
#endif
}

// fuck! the function is difficult to write.
void CALL BSGL_Impl::Gfx_SetTransform(float x, float y, float dx, float dy, float rot, float hscale, float vscale) {
    _render_batch();
    if( 0.0f == hscale || 0.0f == vscale ) {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
#else
        GLfloat m[16];
        m[0]    = 1.f;  m[1]    = 0.f;  m[2]    = 0.f;  m[3]    = 0.f;
        m[4]    = 0.f;  m[5]    = 1.f;  m[6]    = 0.f;  m[7]    = 0.f;
        m[8]    = 0.f;  m[9]    = 0.f;  m[10]   = 1.f;  m[11]   = 0.f;
        m[12]   = 0.f;  m[13]   = 0.f;  m[14]   = 0.f;  m[15]   = 1.f;
        setMatrix(false, m);
#endif
    }else {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(x+dx, y+dy, 0.0f);
        glRotatef(rot, 0.0f, 0.0f, -1.0f);
        glScalef(hscale, vscale, 1.0f);
        glTranslatef(-x, -y, 0.0f);
#else
        GLfloat m[16];
        GLfloat m1[16];
        GLfloat m2[16];
        GLfloat m3[16];
        GLfloat m4[16];
        mt(x+dx, y+dy, m1);
        mr(rot, m2);
        ms(hscale, vscale, m3);
        mt(-x, -y, m4);
        mi(m);
        mxm(m1, m);
        mxm(m2, m);
        mxm(m3, m);
        mxm(m4, m);
        setMatrix(false, m);
#endif
    }
}

bool BSGL_Impl::_GfxInit() {
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
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    //Create IBO in ES2.0
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, VERTEX_BUFFER_SIZE*6/4, indexes, GL_STATIC_DRAW);
#endif

    VertArray = new bsglVertex[VERTEX_BUFFER_SIZE];
    if( 0 == VertArray ) {
        _PostError("Can't create vertex buffer");
        return false;
    }
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    //Create VBO in ES2.0
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_BUFFER_SIZE*sizeof(bsglVertex), VertArray, GL_DYNAMIC_DRAW);
#endif

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
}

void BSGL_Impl::_render_batch(bool bEndScene) {
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    bsglVertex vv;
#endif
    enableTexCoordPointer(true);
    enableColorPointer(true);
    enableVertexPointer(true);
    if( nPrim != 0 ) {
        switch(CurPrimType) {
        case BSGLPRIM_LINES:
            break;
        case BSGLPRIM_TRIPLES:
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            glInterleavedArrays(GL_T2F_C4UB_V3F, 0, VertArray);
#else
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, VERTEX_BUFFER_SIZE*sizeof(bsglVertex), VertArray);
            setTexCoordPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, tx));
            setColorPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, color));
            setVertexPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, x));
#endif
            glDrawArrays(GL_TRIANGLES, 0, nPrim*BSGLPRIM_TRIPLES);//remember to test
            break;
        case BSGLPRIM_QUADS:
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            glInterleavedArrays(GL_T2F_C4UB_V3F, 0, VertArray);
            glDrawElements(GL_TRIANGLES, nPrim*6, GL_UNSIGNED_BYTE, indexes);
#else
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, VERTEX_BUFFER_SIZE*sizeof(bsglVertex), VertArray);
            setTexCoordPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, tx));
            setColorPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, color));
            setVertexPointer(sizeof(bsglVertex), (GLvoid*)offsetof(bsglVertex, x));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
            glDrawElements(GL_TRIANGLES, nPrim*6, GL_UNSIGNED_BYTE, 0);
#endif
            break;
        default:
            break;
        }
        nPrim = 0;
    }
    enableTexCoordPointer(false);
    enableColorPointer(false);
    enableVertexPointer(false);
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
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
#else
            setAddMode(true);
#endif
        }else {
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
            setAddMode(false);
#endif
        }
    }

    CurBlendMode = blend;
}

void _InitOGL() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    initGLSL();
#endif
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
    setAddMode(false);
#endif
}

void _Resize(int width, int height) {
    if( height == 0 ) {
        height = 1;
    }
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);
    glOrtho(0.0, (GLdouble)width, 0.0, (GLdouble)height, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#else
    GLfloat m[16];
    m[0]    = 1.f/(float)width*2.f; m[1]    = 0.f;                      m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;                  m[5]    = -1.f/(float)height*2.f;   m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;                  m[9]    = 0.f;                      m[10]   = -1.f; m[11]   = 0.f;
    m[12]   = -1.f;                 m[13]   = 1.f;                      m[14]   = 0.f;  m[15]   = 1.f;
    setMatrix(true, m);
    m[0]    = 1.f;  m[1]    = 0.f;  m[2]    = 0.f;  m[3]    = 0.f;
    m[4]    = 0.f;  m[5]    = 1.f;  m[6]    = 0.f;  m[7]    = 0.f;
    m[8]    = 0.f;  m[9]    = 0.f;  m[10]   = 1.f;  m[11]   = 0.f;
    m[12]   = 0.f;  m[13]   = 0.f;  m[14]   = 0.f;  m[15]   = 1.f;
    setMatrix(false, m);
#endif
}

int powerOfTwo(int num) {
    int r = 2;
    for (;r<num;) {
        r <<= 1;
    }
    return r;
}

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
    QFile file(filename);
    struct _BMP* bmp;
    if (!file.open(QIODevice::ReadOnly)) {
        return NULL;
    }
    bmp = (struct _BMP*)malloc(sizeof(struct _BMP));
    file.read((char*)&header, sizeof(header));
    file.read((char*)&info, sizeof(info));
    bmp->ow = info.width;
    bmp->oh = info.height;
    bmp->tw = powerOfTwo(bmp->ow);
    bmp->th = powerOfTwo(bmp->oh);
    bmp->data = (unsigned char*)malloc((bmp->tw)*(bmp->th)*sizeof(unsigned int));
    memset(bmp->data, 0, bmp->tw*bmp->th*sizeof(unsigned int));
    file.seek(header.offset);
    for(int h=bmp->oh-1; h>=0; --h) {
        file.read((char*)((bmp->data)+h*bmp->tw*sizeof(unsigned int)), sizeof(unsigned int)*bmp->ow);
    }
    return bmp;
}

void _FreeBMP(struct _BMP* bmp) {
    free(bmp->data);
}
