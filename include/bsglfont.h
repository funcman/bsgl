/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2015, Buggy-Mushroom Studio
**
** bsglFont class header
*/

#ifndef BSGLFONT_H
#define BSGLFONT_H

#include "bsgl.h"
#include <ft2build.h>
#include FT_FREETYPE_H

struct  bsglGlyph;

class bsglFont {
public:
    bsglFont(char const* filename, int size=16);
    ~bsglFont();

    void BeginDrawTexture(HTEXTURE tex, int x, int y, int height);
    void DrawGlyph(wchar_t wc);
    void EndDrawTexture();

protected:
    bsglFont();
    static BSGL* bsgl;

    FT_Library  ft;
    FT_Face     face;
    void*       font_data;

    HTEXTURE    thetex;
    int         tex_w;
    int         tex_h;
    int         origin_x;
    int         origin_y;
    int         line_height;
    int         last_advance;
    int         num_lines;
    DWORD*      data;
};

#endif//BSGLFONT_H
