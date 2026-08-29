/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglFont class header
*/

#ifndef BSGLFONT_H
#define BSGLFONT_H

#include "bsgl.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <list>
#include <map>

class  bsglTextMesh;

struct bsglGlyph {
    HTEXTURE    tex;        // atlas page holding the rasterized glyph
    float       u0, v0;     // atlas UV of the top-left corner
    float       u1, v1;     // atlas UV of the bottom-right corner
    float       w, h;       // glyph bitmap size in pixels
    int         left, top;  // bitmap_left / bitmap_top (bearing)
    float       advance;    // horizontal advance in pixels
};

class bsglFont {
public:
    bsglFont(char const* filename, int size=16, int atlas_size=512);
    ~bsglFont();

    // immediate-mode API: rasterize glyphs straight into a caller-owned
    // texture (no caching, re-rasterizes on every call)
    void BeginDrawTexture(HTEXTURE tex, int x, int y, int height);
    void DrawGlyph(wchar_t wc);
    void EndDrawTexture();

    // cached API: glyphs are rasterized once into an internal atlas
    // (pages of atlas_size x atlas_size, chained in a std::list; a new
    // page is appended whenever the current one is full) and text is
    // drawn as tinted quads that sample the atlas via UV
    bool        Loaded() const;
    int         GetSize() const          { return size; }
    float       GetLineHeight() const    { return line_height; }
    float       GetAscender() const      { return ascender; }
    float       GetTextWidth(char const* text);
    void        DrawText(float x, float y, char const* text,
                         DWORD color=0xFFFFFFFF, int blend=BLEND_DEFAULT);
    // build a reusable mesh (quads referencing the atlas); the mesh can
    // then be rendered any number of times without re-layout
    void        RenderText(bsglTextMesh* mesh, char const* text);

    const bsglGlyph* GetGlyph(wchar_t wc);

protected:
    bsglFont();
    static BSGL* bsgl;

    struct AtlasPage {
        HTEXTURE    tex;
        int         w, h;
        int         cursor_x, cursor_y, row_h;
    };

    AtlasPage*  NewPage();

    FT_Library  ft;
    FT_Face     face;
    void*       font_data;
    int         size;
    int         atlas_size;

    // glyph atlas cache
    std::list<AtlasPage>            pages;
    std::map<wchar_t, bsglGlyph>    glyphs;

    // face metrics (pixels)
    float       ascender;
    float       descender;
    float       line_height;

    // legacy immediate-mode scratch state
    HTEXTURE    thetex;
    int         tex_w;
    int         tex_h;
    int         origin_x;
    int         origin_y;
    int         im_line_height;
    int         last_advance;
    int         num_lines;
    DWORD*      data;
};

#endif//BSGLFONT_H
