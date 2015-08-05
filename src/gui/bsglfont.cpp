/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (C) 2008-2015, Buggy-Mushroom Studio
 **
 ** bsglFont class implementation
 */

#include "bsglfont.h"
#include <string>
#include <ftglyph.h>
#include <QFile>

BSGL* bsglFont::bsgl=0;

bsglFont::bsglFont(char const* filename, int size) {
    bsgl = bsglCreate(BSGL_VERSION);
    FT_Init_FreeType(&ft);
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    font_data = new unsigned char[file.size()];
    file.read((char*)font_data, (qint64)file.size());
    FT_New_Memory_Face(ft, (FT_Byte const*)font_data, file.size(), 0, &face);

    FT_Set_Pixel_Sizes(face, size, 0);
}

bsglFont::~bsglFont() {
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    delete[] font_data;
    bsgl->Release();
}

void bsglFont::BeginDrawTexture(HTEXTURE tex, int ox, int oy, int height) {
    origin_x = ox;
    origin_y = oy;
    thetex = tex;
    tex_w = bsgl->Texture_GetWidth(tex);
    tex_h = bsgl->Texture_GetHeight(tex);
    line_height = height;
    data = bsgl->Texture_LoadData(tex);
    if (!data) {
        data = new DWORD[tex_w*tex_h];
        memset(data, 255, tex_w*tex_h*4);
    }
    last_advance = 0;
    num_lines = 1;
}

void bsglFont::DrawGlyph(wchar_t wc) {
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
    int y = origin_y - face->glyph->bitmap_top + (num_lines - 1) * line_height;

    unsigned char* alpha = (unsigned char*)(data)+(y*tex_w+x)*4;

    for (int i=0; i<face->glyph->bitmap.rows; ++i) {
        for (int j=0; j<face->glyph->bitmap.width; ++j) {
            if (&alpha[(i*tex_w+j)*4] < (unsigned char*)data) continue;
            if (&alpha[(i*tex_w+j)*4] >= (unsigned char*)(data+tex_w*tex_h)) continue;
            float a = face->glyph->bitmap.buffer[i*face->glyph->bitmap.width+j];
            alpha[(i*tex_w+j)*4+1]  = ((float)alpha[(i*tex_w+j)*4+1])*(255.f-a)/255.f;
            alpha[(i*tex_w+j)*4+2]  = ((float)alpha[(i*tex_w+j)*4+2])*(255.f-a)/255.f;
            alpha[(i*tex_w+j)*4+3]  = ((float)alpha[(i*tex_w+j)*4+3])*(255.f-a)/255.f;
        }
    }

    last_advance += (face->glyph->advance.x >> 6);
}

void bsglFont::EndDrawTexture() {
    bsgl->Texture_Update(thetex, data, 0, 0, tex_w, tex_h);
}
