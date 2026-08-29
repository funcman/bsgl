/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (c) 2008-2026 Buggy-Mushroom Studio
 **
 ** bsglFont class implementation
 */

#include "bsglfont.h"
#include "bsgltextmesh.h"
#include <string>
#include <stdio.h>

BSGL* bsglFont::bsgl=nullptr;

bsglFont::bsglFont(char const* filename, int size, int atlas_size) {
    bsgl = bsglCreate(BSGL_VERSION);
    FT_Init_FreeType(&ft);
    this->size = size;
    this->atlas_size = atlas_size > 0 ? atlas_size : 512;
    ascender = descender = line_height = 0.0f;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        font_data = nullptr;
        face = nullptr;
        return;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    font_data = new unsigned char[file_size];
    fread(font_data, file_size, 1, file);
    fclose(file);
    FT_New_Memory_Face(ft, (FT_Byte const*)font_data, file_size, 0, &face);

    FT_Set_Pixel_Sizes(face, size, 0);
    if (face) {
        ascender    = face->size->metrics.ascender >> 6;
        descender   = face->size->metrics.descender >> 6;
        line_height = (face->size->metrics.height >> 6);
        if (line_height < 1.0f) {
            line_height = (float)((size * 3) / 2);
        }
    }

    // atlas pages are created lazily: the font may be constructed before
    // System_Initiate, when no GL context exists yet
    data = nullptr;
    thetex = 0;
}

bsglFont::~bsglFont() {
    if (face) {
        FT_Done_Face(face);
    }
    FT_Done_FreeType(ft);
    delete[] font_data;
    for (std::list<AtlasPage>::iterator it = pages.begin(); it != pages.end(); ++it) {
        if (it->tex) {
            bsgl->Texture_Free(it->tex);
        }
    }
    pages.clear();
    bsgl->Release();
}

bool bsglFont::Loaded() const {
    return face != nullptr;
}

// Legacy immediate-mode API ------------------------------------------

void bsglFont::BeginDrawTexture(HTEXTURE tex, int ox, int oy, int height) {
    if (!face) {
        data = nullptr;
        return;
    }
    origin_x = ox;
    origin_y = oy;
    thetex = tex;
    tex_w = bsgl->Texture_GetWidth(tex);
    tex_h = bsgl->Texture_GetHeight(tex);
    im_line_height = height;
    data = bsgl->Texture_LoadData(tex);
    if (!data) {
        data = new DWORD[tex_w*tex_h];
        memset(data, 0, tex_w*tex_h*4);
    }
    last_advance = 0;
    num_lines = 1;
}

void bsglFont::DrawGlyph(wchar_t wc) {
    if (!face || !data) {
        return;
    }
    FT_UInt index = FT_Get_Char_Index(face, wc);
    FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        FT_Render_Glyph(face->glyph, ft_render_mode_normal);
    }

    int x = origin_x + face->glyph->bitmap_left + last_advance;

    if (x + (face->glyph->advance.x >> 6) > tex_w) {
        last_advance = 0;
        ++num_lines;
        x = origin_x + face->glyph->bitmap_left;
    }
    int y = origin_y - face->glyph->bitmap_top + (num_lines - 1) * im_line_height;

    unsigned char* alpha = (unsigned char*)(data)+(y*tex_w+x)*4;

    for (int i=0; i<face->glyph->bitmap.rows; ++i) {
        for (int j=0; j<face->glyph->bitmap.width; ++j) {
            if (&alpha[(i*tex_w+j)*4] < (unsigned char*)data) continue;
            if (&alpha[(i*tex_w+j)*4] >= (unsigned char*)(data+tex_w*tex_h)) continue;
            float a = face->glyph->bitmap.buffer[i*face->glyph->bitmap.width+j]/255.f;
            unsigned char* px = &alpha[(i*tex_w+j)*4];
            // composite a white glyph over the existing pixel:
            // rgb blends toward white, alpha toward opaque
            px[0] = (unsigned char)(px[0]*(1.f-a) + 255.f*a);
            px[1] = (unsigned char)(px[1]*(1.f-a) + 255.f*a);
            px[2] = (unsigned char)(px[2]*(1.f-a) + 255.f*a);
            px[3] = (unsigned char)(px[3] + (255.f-px[3])*a);
        }
    }

    last_advance += (face->glyph->advance.x >> 6);
}

void bsglFont::EndDrawTexture() {
    if (!data) {
        return;
    }
    bsgl->Texture_Update(thetex, data, 0, 0, tex_w, tex_h);
    bsgl->Texture_FreeData(data);
    data = nullptr;
}

// Glyph atlas cache ---------------------------------------------------

bsglFont::AtlasPage* bsglFont::NewPage() {
    AtlasPage page;
    page.tex = 0;
    page.w = atlas_size;
    page.h = atlas_size;
    page.cursor_x = 0;
    page.cursor_y = 0;
    page.row_h = 0;

    page.tex = bsgl->Texture_Create(page.w, page.h);
    // Texture_Create leaves the pixels uninitialized; clear the page to
    // fully transparent so unused atlas space never leaks into glyphs
    int tw = bsgl->Texture_GetWidth(page.tex);
    int th = bsgl->Texture_GetHeight(page.tex);
    DWORD* pixels = bsgl->Texture_CreateData(tw, th);
    if (pixels) {
        memset(pixels, 0, tw*th*sizeof(DWORD));
        bsgl->Texture_Update(page.tex, pixels, 0, 0, tw, th);
        bsgl->Texture_FreeData(pixels);
    }

    pages.push_back(page);
    return &pages.back();
}

const bsglGlyph* bsglFont::GetGlyph(wchar_t wc) {
    if (!face) {
        return nullptr;
    }

    std::map<wchar_t, bsglGlyph>::iterator found = glyphs.find(wc);
    if (found != glyphs.end()) {
        return &found->second;
    }

    FT_UInt index = FT_Get_Char_Index(face, wc);
    FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        FT_Render_Glyph(face->glyph, ft_render_mode_normal);
    }

    bsglGlyph glyph;
    glyph.w        = (float)face->glyph->bitmap.width;
    glyph.h        = (float)face->glyph->bitmap.rows;
    glyph.left     = face->glyph->bitmap_left;
    glyph.top      = face->glyph->bitmap_top;
    glyph.advance  = (float)(face->glyph->advance.x >> 6);

    // whitespace glyphs carry no pixels, only advance
    if (glyph.w < 1.0f || glyph.h < 1.0f) {
        glyph.tex = 0;
        glyph.u0 = glyph.v0 = glyph.u1 = glyph.v1 = 0.0f;
        return &glyphs.insert(std::make_pair(wc, glyph)).first->second;
    }

    // find a page with room (shelf packing, 1px gutter against linear
    // filter bleeding between neighboring glyphs)
    int gw = (int)glyph.w + 1;
    int gh = (int)glyph.h + 1;
    AtlasPage* page = pages.empty() ? nullptr : &pages.back();
    if (!page || page->cursor_x + gw > page->w) {
        if (page && page->cursor_y + page->row_h + gh <= page->h) {
            // start a new row on the current page
            page->cursor_x = 0;
            page->cursor_y += page->row_h;
            page->row_h = 0;
        } else {
            page = NewPage();
        }
    }
    if (page->cursor_y + gh > page->h) {
        // glyph taller than a whole fresh page row cannot be cached
        return nullptr;
    }

    // convert the A8 coverage bitmap to white RGBA pixels
    DWORD* buf = new DWORD[(int)glyph.w * (int)glyph.h];
    for (int i=0; i<(int)glyph.h; ++i) {
        for (int j=0; j<(int)glyph.w; ++j) {
            unsigned char a = face->glyph->bitmap.buffer[i*(int)glyph.w+j];
            // white RGB with the coverage in alpha, so the fixed-function
            // texture*vertex modulation tints the glyph with the quad color
            buf[i*(int)glyph.w+j] = RGBA(0xFF, 0xFF, 0xFF, a);
        }
    }
    bsgl->Texture_Update(page->tex, buf, page->cursor_x, page->cursor_y,
                         (int)glyph.w, (int)glyph.h);
    delete[] buf;

    int tw = bsgl->Texture_GetWidth(page->tex);
    int th = bsgl->Texture_GetHeight(page->tex);
    glyph.tex = page->tex;
    glyph.u0 = (float)page->cursor_x / tw;
    glyph.v0 = (float)page->cursor_y / th;
    glyph.u1 = (float)(page->cursor_x + (int)glyph.w) / tw;
    glyph.v1 = (float)(page->cursor_y + (int)glyph.h) / th;

    page->cursor_x += gw;
    if (gh > page->row_h) {
        page->row_h = gh;
    }

    return &glyphs.insert(std::make_pair(wc, glyph)).first->second;
}

float bsglFont::GetTextWidth(char const* text) {
    float width = 0.0f;
    for (char const* p = text; *p; ++p) {
        const bsglGlyph* glyph = GetGlyph((wchar_t)(unsigned char)*p);
        if (glyph) {
            width += glyph->advance;
        }
    }
    return width;
}

void bsglFont::DrawText(float x, float y, char const* text,
                        DWORD color, int blend) {
    if (!face) {
        return;
    }

    // x,y is the top-left of the line; place glyphs relative to the
    // baseline derived from the face metrics
    float baseline = y + ascender;
    float pen = x;

    for (char const* p = text; *p; ++p) {
        const bsglGlyph* glyph = GetGlyph((wchar_t)(unsigned char)*p);
        if (!glyph || !glyph->tex) {
            if (glyph) {
                pen += glyph->advance;
            }
            continue;
        }

        float gx = pen + glyph->left;
        float gy = baseline - glyph->top;

        bsglQuad quad;
        quad.tex = glyph->tex;
        quad.blend = blend;
        for (int i=0; i<4; ++i) {
            quad.v[i].color = color;
            quad.v[i].z = 0.5f;
        }
        quad.v[0].x = gx;          quad.v[0].y = gy;          quad.v[0].tx = glyph->u0; quad.v[0].ty = glyph->v0;
        quad.v[1].x = gx+glyph->w; quad.v[1].y = gy;          quad.v[1].tx = glyph->u1; quad.v[1].ty = glyph->v0;
        quad.v[2].x = gx+glyph->w; quad.v[2].y = gy+glyph->h; quad.v[2].tx = glyph->u1; quad.v[2].ty = glyph->v1;
        quad.v[3].x = gx;          quad.v[3].y = gy+glyph->h; quad.v[3].tx = glyph->u0; quad.v[3].ty = glyph->v1;
        bsgl->Gfx_RenderQuad(&quad);

        pen += glyph->advance;
    }
}

void bsglFont::RenderText(bsglTextMesh* mesh, char const* text) {
    if (mesh) {
        mesh->Build(this, text);
    }
}
