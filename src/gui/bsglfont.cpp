/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (c) 2008-2026 Buggy-Mushroom Studio
 **
 ** bsglFont class implementation
 */

#include "bsglfont.h"
#include <string>
#include <stdio.h>

BSGL* bsglFont::bsgl=nullptr;

bsglFont::bsglFont(char const* filename, int size) {
    bsgl = bsglCreate(BSGL_VERSION);
    FT_Init_FreeType(&ft);
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
}

bsglFont::~bsglFont() {
    if (face) {
        FT_Done_Face(face);
    }
    FT_Done_FreeType(ft);
    delete[] font_data;
    bsgl->Release();
}

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
    line_height = height;
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
    int y = origin_y - face->glyph->bitmap_top + (num_lines - 1) * line_height;

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
