/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglSprite9Slice util class header
*/

#ifndef BSGLSPRITE9_H
#define BSGLSPRITE9_H

#include "bsgl.h"

class bsglSprite9Slice {
public:
    bsglSprite9Slice(HTEXTURE tex, float x, float y, float w, float h);
    bsglSprite9Slice(const bsglSprite9Slice& spr);
    ~bsglSprite9Slice() { bsgl->Release(); }

    // 设置九宫格边距（像素单位，从四边向内）
    void SetInsets(float left, float top, float right, float bottom);
    // 渲染到指定矩形区域
    void Render(float x, float y, float width, float height);

    void SetTexture(HTEXTURE tex);
    void SetTextureRect(float x, float y, float w, float h);
    void SetColor(DWORD col, int i=-1);
    void SetZ(float z, int i=-1);
    void SetBlendMode(int blend);

    HTEXTURE GetTexture() const;
    void GetTextureRect(float* x, float* y, float* w, float* h) const;
    DWORD GetColor(int i=0) const;
    float GetZ(int i=0) const;
    int GetBlendMode() const;

    float GetWidth() const;
    float GetHeight() const;

protected:
    static BSGL* bsgl;

    bsglQuad    quad;
    float       tx, ty, width, height;
    float       tex_width, tex_height;
    float       insetL, insetT, insetR, insetB;
};

#endif//BSGLSPRITE9_H
