/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglSprite9Slice util class implementation
*/

#include "bsglsprite9.h"
#include <string.h>

BSGL *bsglSprite9Slice::bsgl=nullptr;

bsglSprite9Slice::bsglSprite9Slice(HTEXTURE tex, float x, float y, float w, float h) {
    bsgl = bsglCreate(BSGL_VERSION);

    tx      = x;
    ty      = y;
    width   = w;
    height  = h;

    if (tex) {
        tex_width   = (float)bsgl->Texture_GetWidth(tex);
        tex_height  = (float)bsgl->Texture_GetHeight(tex);
    } else {
        tex_width   = 1.0f;
        tex_height  = 1.0f;
    }

    insetL = insetT = insetR = insetB = 0.0f;

    memset(&quad, 0, sizeof(quad));
    quad.tex    = tex;
    quad.blend  = BLEND_DEFAULT;

    SetTextureRect(x, y, w, h);
    SetColor(RGBA(0xFF, 0xFF, 0xFF, 0xFF));
    SetZ(0.5f);
}

bsglSprite9Slice::bsglSprite9Slice(const bsglSprite9Slice& spr) {
    memcpy(this, &spr, sizeof(bsglSprite9Slice));
    bsgl = bsglCreate(BSGL_VERSION);
}

void bsglSprite9Slice::SetInsets(float left, float top, float right, float bottom) {
    insetL = left;
    insetT = top;
    insetR = right;
    insetB = bottom;
}

void bsglSprite9Slice::Render(float x, float y, float width, float height) {
    // destination x/y split lines
    float dx[4] = { x, x + insetL, x + width - insetR, x + width };
    float dy[4] = { y, y + insetT, y + height - insetB, y + height };

    // source u/v split lines in normalized texture coords
    float du[4] = {
        quad.v[0].tx,
        (tx + insetL) / tex_width,
        (tx + this->width - insetR) / tex_width,
        quad.v[2].tx
    };
    float dv[4] = {
        quad.v[0].ty,
        (ty + insetT) / tex_height,
        (ty + this->height - insetB) / tex_height,
        quad.v[2].ty
    };

    // corners are drawn at inset size, edges stretch along one axis, center stretches both
    bsglQuad q = quad;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            q.v[0].x = dx[col];     q.v[0].y = dy[row];
            q.v[1].x = dx[col+1];   q.v[1].y = dy[row];
            q.v[2].x = dx[col+1];   q.v[2].y = dy[row+1];
            q.v[3].x = dx[col];     q.v[3].y = dy[row+1];

            q.v[0].tx = du[col];    q.v[0].ty = dv[row];
            q.v[1].tx = du[col+1];  q.v[1].ty = dv[row];
            q.v[2].tx = du[col+1];  q.v[2].ty = dv[row+1];
            q.v[3].tx = du[col];    q.v[3].ty = dv[row+1];

            bsgl->Gfx_RenderQuad(&q);
        }
    }
}

void bsglSprite9Slice::SetTexture(HTEXTURE tex) {
    quad.tex = tex;

    if (tex) {
        tex_width   = (float)bsgl->Texture_GetWidth(tex);
        tex_height  = (float)bsgl->Texture_GetHeight(tex);
    } else {
        tex_width   = 1.0f;
        tex_height  = 1.0f;
    }

    SetTextureRect(tx, ty, width, height);
}

void bsglSprite9Slice::SetTextureRect(float x, float y, float w, float h) {
    tx = x;
    ty = y;
    width   = w;
    height  = h;

    quad.v[0].tx = tx/tex_width;        quad.v[0].ty = ty/tex_height;
    quad.v[1].tx = (tx+w)/tex_width;    quad.v[1].ty = ty/tex_height;
    quad.v[2].tx = (tx+w)/tex_width;    quad.v[2].ty = (ty+h)/tex_height;
    quad.v[3].tx = tx/tex_width;        quad.v[3].ty = (ty+h)/tex_height;
}

void bsglSprite9Slice::SetColor(DWORD col, int i) {
    if (-1 == i) {
        quad.v[0].color =
        quad.v[1].color =
        quad.v[2].color =
        quad.v[3].color = col;
    } else {
        quad.v[i].color = col;
    }
}

void bsglSprite9Slice::SetZ(float z, int i) {
    if (-1 == i) {
        quad.v[0].z =
        quad.v[1].z =
        quad.v[2].z =
        quad.v[3].z = z;
    } else {
        quad.v[i].z = z;
    }
}

void bsglSprite9Slice::SetBlendMode(int blend) {
    quad.blend = blend;
}

HTEXTURE bsglSprite9Slice::GetTexture() const {
    return quad.tex;
}

void bsglSprite9Slice::GetTextureRect(float* x, float* y, float* w, float* h) const {
    *x = tx;
    *y = ty;
    *w = width;
    *h = height;
}

DWORD bsglSprite9Slice::GetColor(int i) const {
    return quad.v[i].color;
}

float bsglSprite9Slice::GetZ(int i) const {
    return quad.v[i].z;
}

int bsglSprite9Slice::GetBlendMode() const {
    return quad.blend;
}

float bsglSprite9Slice::GetWidth() const {
    return width;
}

float bsglSprite9Slice::GetHeight() const {
    return height;
}
