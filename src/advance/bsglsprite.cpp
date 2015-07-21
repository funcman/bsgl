/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglSprite util class implementation
*/

#include "bsglsprite.h"
#include <string.h>
#include <math.h>
#include "bsglrect.h"

BSGL *bsglSprite::bsgl=0;

bsglSprite::bsglSprite(HTEXTURE tex, float x, float y, float w, float h) {
    float texx1, texy1, texx2, texy2;

    bsgl = bsglCreate(BSGL_VERSION);

    tx      = x;
    ty      = y;
    width   = w;
    height  = h;

    if(tex)    {
        tex_width   = (float)bsgl->Texture_GetWidth(tex);
        tex_height  = (float)bsgl->Texture_GetHeight(tex);
    }else {
        tex_width   = 1.0f;
        tex_height  = 1.0f;
    }

    hotX        = 0;
    hotY        = 0;
    bXFlip      = false;
    bYFlip      = false;
    bHSFlip     = false;
    quad.tex    = tex;

    texx1   = x/tex_width;
    texy1   = y/tex_height;
    texx2   = (x+w)/tex_width;
    texy2   = (y+h)/tex_height;

    quad.v[0].tx = texx1; quad.v[0].ty = texy1;
    quad.v[1].tx = texx2; quad.v[1].ty = texy1;
    quad.v[2].tx = texx2; quad.v[2].ty = texy2;
    quad.v[3].tx = texx1; quad.v[3].ty = texy2;

    quad.v[0].z =
    quad.v[1].z =
    quad.v[2].z =
    quad.v[3].z = 0.5f;

    quad.v[0].color =
    quad.v[1].color =
    quad.v[2].color =
    quad.v[3].color = RGBA(0xFF, 0xFF, 0xFF, 0xFF);

    quad.blend=BLEND_DEFAULT;
}

bsglSprite::bsglSprite(const bsglSprite& spr) {
    memcpy(this, &spr, sizeof(bsglSprite));
    bsgl = bsglCreate(BSGL_VERSION);
}

void bsglSprite::Render(float x, float y) {
    float tmp_x1, tmp_y1, tmp_x2, tmp_y2;

    tmp_x1 = x-hotX;
    tmp_y1 = y-hotY;
    tmp_x2 = x+width-hotX;
    tmp_y2 = y+height-hotY;

    quad.v[0].x = tmp_x1; quad.v[0].y = tmp_y1;
    quad.v[1].x = tmp_x2; quad.v[1].y = tmp_y1;
    quad.v[2].x = tmp_x2; quad.v[2].y = tmp_y2;
    quad.v[3].x = tmp_x1; quad.v[3].y = tmp_y2;

    bsgl->Gfx_RenderQuad(&quad);
}

void bsglSprite::RenderEx(float x, float y, float rot, float hscale, float vscale) {
    float tx1, ty1, tx2, ty2;
    float sint, cost;

    if( 0.0f == vscale ) {
        vscale = hscale;
    }

    tx1 = -hotX*hscale;
    ty1 = -hotY*vscale;
    tx2 = (width-hotX)*hscale;
    ty2 = (height-hotY)*vscale;

    if( rot != 0.0f ) {
        sint = sinf(rot);
        cost = cosf(rot);
        quad.v[0].x = tx1*cost - ty1*sint + x;
        quad.v[0].y = tx1*sint + ty1*cost + y;
        quad.v[1].x = tx2*cost - ty1*sint + x;
        quad.v[1].y = tx2*sint + ty1*cost + y;
        quad.v[2].x = tx2*cost - ty2*sint + x;
        quad.v[2].y = tx2*sint + ty2*cost + y;
        quad.v[3].x = tx1*cost - ty2*sint + x;
        quad.v[3].y = tx1*sint + ty2*cost + y;
    }else {
        quad.v[0].x = tx1 + x; quad.v[0].y = ty1 + y;
        quad.v[1].x = tx2 + x; quad.v[1].y = ty1 + y;
        quad.v[2].x = tx2 + x; quad.v[2].y = ty2 + y;
        quad.v[3].x = tx1 + x; quad.v[3].y = ty2 + y;
    }

    bsgl->Gfx_RenderQuad(&quad);
}

void bsglSprite::RenderStretch(float x1, float y1, float x2, float y2) {
    quad.v[0].x = x1; quad.v[0].y = y1;
    quad.v[1].x = x2; quad.v[1].y = y1;
    quad.v[2].x = x2; quad.v[2].y = y2;
    quad.v[3].x = x1; quad.v[3].y = y2;

    bsgl->Gfx_RenderQuad(&quad);
}

void bsglSprite::Render4V(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
    quad.v[0].x = x0; quad.v[0].y = y0;
    quad.v[1].x = x1; quad.v[1].y = y1;
    quad.v[2].x = x2; quad.v[2].y = y2;
    quad.v[3].x = x3; quad.v[3].y = y3;

    bsgl->Gfx_RenderQuad(&quad);
}

void bsglSprite::SetTexture(HTEXTURE tex) {
    float tx1, ty1, tx2, ty2;
    float tw, th;

    quad.tex = tex;

    if( tex ) {
        tw = (float)bsgl->Texture_GetWidth(tex);
        th = (float)bsgl->Texture_GetHeight(tex);
    }else {
        tw = 1.0f;
        th = 1.0f;
    }

    if( tw!=tex_width || th!=tex_height ) {
        tx1 = quad.v[0].tx*tex_width;
        ty1 = quad.v[0].ty*tex_height;
        tx2 = quad.v[2].tx*tex_width;
        ty2 = quad.v[2].ty*tex_height;

        tex_width   = tw;
        tex_height  = th;

        tx1/=tw; ty1/=th;
        tx2/=tw; ty2/=th;

        quad.v[0].tx = tx1; quad.v[0].ty = ty1;
        quad.v[1].tx = tx2; quad.v[1].ty = ty1;
        quad.v[2].tx = tx2; quad.v[2].ty = ty2;
        quad.v[3].tx = tx1; quad.v[3].ty = ty2;
    }
}

void bsglSprite::SetTextureRect(float x, float y, float w, float h, bool adjSize) {
    float tx1, ty1, tx2, ty2;
    bool bX, bY, bHS;

    tx = x;
    ty = y;

    if( adjSize ) {
        width   = w;
        height  = h;
    }

    tx1 = tx/tex_width;     ty1 = ty/tex_height;
    tx2 = (tx+w)/tex_width; ty2 = (ty+h)/tex_height;

    quad.v[0].tx = tx1; quad.v[0].ty = ty1;
    quad.v[1].tx = tx2; quad.v[1].ty = ty1;
    quad.v[2].tx = tx2; quad.v[2].ty = ty2;
    quad.v[3].tx = tx1; quad.v[3].ty = ty2;

    bX      = bXFlip;
    bY      = bYFlip;
    bHS     = bHSFlip;
    bXFlip  = false;
    bYFlip  = false;
    SetFlip(bX, bY, bHS);
}

void bsglSprite::SetColor(DWORD col, int i) {
    if( -1 == i ) {
        quad.v[0].color =
        quad.v[1].color =
        quad.v[2].color =
        quad.v[3].color = col;
    }else {
        quad.v[i].color = col;
    }
}

void bsglSprite::SetZ(float z, int i) {
    if( -1 == i ) {
        quad.v[0].z =
        quad.v[1].z =
        quad.v[2].z =
        quad.v[3].z = z;
    }else {
        quad.v[i].z = z;
    }
}

void bsglSprite::SetBlendMode(int blend) {
    quad.blend = blend;
}

void bsglSprite::SetHotSpot(float x, float y) {
    this->hotX = x;
    this->hotY = y;
}

void bsglSprite::SetFlip(bool bX, bool bY, bool bHotSpot) {
    float tx, ty;

    if( bHSFlip && bXFlip ) hotX = width - hotX;
    if( bHSFlip && bYFlip ) hotY = height - hotY;

    bHSFlip = bHotSpot;

    if( bHSFlip && bXFlip ) hotX = width - hotX;
    if( bHSFlip && bYFlip ) hotY = height - hotY;

    if( bX != bXFlip ) {
        tx=quad.v[0].tx; quad.v[0].tx=quad.v[1].tx; quad.v[1].tx=tx;
        ty=quad.v[0].ty; quad.v[0].ty=quad.v[1].ty; quad.v[1].ty=ty;
        tx=quad.v[3].tx; quad.v[3].tx=quad.v[2].tx; quad.v[2].tx=tx;
        ty=quad.v[3].ty; quad.v[3].ty=quad.v[2].ty; quad.v[2].ty=ty;

        bXFlip = !bXFlip;
    }

    if( bY != bYFlip ) {
        tx=quad.v[0].tx; quad.v[0].tx=quad.v[3].tx; quad.v[3].tx=tx;
        ty=quad.v[0].ty; quad.v[0].ty=quad.v[3].ty; quad.v[3].ty=ty;
        tx=quad.v[1].tx; quad.v[1].tx=quad.v[2].tx; quad.v[2].tx=tx;
        ty=quad.v[1].ty; quad.v[1].ty=quad.v[2].ty; quad.v[2].ty=ty;

        bYFlip = !bYFlip;
    }
}

HTEXTURE bsglSprite::GetTexture() const {
    return this->quad.tex;
}

void bsglSprite::GetTextureRect(float* x, float* y, float* w, float* h) const {
    *x = tx;
    *y = ty;
    *w = width;
    *h = height;
}

DWORD bsglSprite::GetColor(int i) const {
    return this->quad.v[i].color;
}

float bsglSprite::GetZ(int i) const {
    return this->quad.v[i].z;
}

int bsglSprite::GetBlendMode() const {
    return this->quad.blend;
}

void bsglSprite::GetHotSpot(float* x, float* y) const {
    *x = hotX;
    *y = hotY;
}

void bsglSprite::GetFlip(bool* bX, bool* bY) const {
    *bX = bXFlip;
    *bY = bYFlip;
}

float bsglSprite::GetWidth() const {
    return width;
}

float bsglSprite::GetHeight() const {
    return height;
}

bsglRect* bsglSprite::GetBoundingBox(float x, float y, bsglRect* rect) const {
    rect->Set(x-hotX, y-hotY, x-hotX+width, y-hotY+height);
    return rect;
}

bsglRect* bsglSprite::GetBoundingBoxEx(float x, float y, float rot, float vscale, float hscale, bsglRect* rect) const {
    float tx1, ty1, tx2, ty2;
    float sint, cost;

    rect->Clear();

    tx1 = -hotX * hscale;
    ty1 = -hotY * vscale;
    tx2 = (width-hotX) * hscale;
    ty2 = (height-hotY) * vscale;

    if( rot != 0.0f ) {
        cost = cosf(rot);
        sint = sinf(rot);

        rect->Encapsulate(tx1*cost - ty1*sint + x, tx1*sint + ty1*cost + y);
        rect->Encapsulate(tx2*cost - ty1*sint + x, tx2*sint + ty1*cost + y);
        rect->Encapsulate(tx2*cost - ty2*sint + x, tx2*sint + ty2*cost + y);
        rect->Encapsulate(tx1*cost - ty2*sint + x, tx1*sint + ty2*cost + y);
    }else {
        rect->Encapsulate(tx1 + x, ty1 + y);
        rect->Encapsulate(tx2 + x, ty1 + y);
        rect->Encapsulate(tx2 + x, ty2 + y);
        rect->Encapsulate(tx1 + x, ty2 + y);
    }

    return rect;
}
