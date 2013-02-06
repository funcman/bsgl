/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglSprite util class header
*/

#ifndef BSGLSPRITE_H
#define BSGLSPRITE_H

#include "bsgl.h"

class bsglRect;

class bsglSprite {
public:
    bsglSprite(HTEXTURE tex, float x, float y, float w, float h);
    bsglSprite(const bsglSprite& spr);
    ~bsglSprite() { bsgl->Release(); }

    void Render(float x, float y);
    void RenderEx(float x, float y, float rot, float hscale=1.0f, float vscale=0.0f);
    void RenderStretch(float x1, float y1, float x2, float y2);
    void Render4V(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);

    void SetTexture(HTEXTURE tex);
    void SetTextureRect(float x, float y, float w, float h, bool adjSize=true);
    void SetColor(DWORD col, int i=-1);
    void SetZ(float z, int i=-1);
    void SetBlendMode(int blend);
    void SetHotSpot(float x, float y);
    void SetFlip(bool bX, bool bY, bool bHotSpot=false);

    HTEXTURE GetTexture() const;
    void GetTextureRect(float* x, float* y, float* w, float* h) const;
    DWORD GetColor(int i=0) const;
    float GetZ(int i=0) const;
    int GetBlendMode() const;
    void GetHotSpot(float* x, float* y) const;
    void GetFlip(bool* bX, bool* bY) const;

    float GetWidth() const;
    float GetHeight() const;
    bsglRect* GetBoundingBox(float x, float y, bsglRect* rect) const;
    bsglRect* GetBoundingBoxEx(float x, float y, float rot, float hscale, float vscale, bsglRect* rect) const;

protected:
    bsglSprite();
    static BSGL* bsgl;

    bsglQuad    quad;
    float       tx, ty, width, height;
    float       tex_width, tex_height;
    float       hotX, hotY;
    bool        bXFlip, bYFlip, bHSFlip;
};

#endif//BSGLSPRITE_H

