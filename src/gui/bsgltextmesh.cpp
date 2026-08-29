/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (c) 2008-2026 Buggy-Mushroom Studio
 **
 ** bsglTextMesh class implementation
 */

#include "bsgltextmesh.h"
#include "bsglfont.h"

BSGL* bsglTextMesh::bsgl=nullptr;

bsglTextMesh::bsglTextMesh() {
    bsgl = bsglCreate(BSGL_VERSION);
    width = 0.0f;
    height = 0.0f;
}

void bsglTextMesh::Build(bsglFont* font, char const* text) {
    groups.clear();
    width = 0.0f;
    height = 0.0f;
    if (!font || !text) {
        return;
    }

    float baseline = font->GetAscender();
    height = font->GetLineHeight();
    float pen = 0.0f;

    for (char const* p = text; *p; ++p) {
        const bsglGlyph* glyph = font->GetGlyph((wchar_t)(unsigned char)*p);
        if (!glyph) {
            continue;
        }
        pen += glyph->advance;
        if (!glyph->tex) {
            continue;
        }

        QuadGroup* group = nullptr;
        for (size_t i=0; i<groups.size(); ++i) {
            if (groups[i].tex == glyph->tex) {
                group = &groups[i];
                break;
            }
        }
        if (!group) {
            QuadGroup g;
            g.tex = glyph->tex;
            g.blend = BLEND_DEFAULT;
            groups.push_back(g);
            group = &groups.back();
        }

        float gx = pen - glyph->advance + glyph->left;
        float gy = baseline - glyph->top;

        bsglQuad quad;
        quad.tex = glyph->tex;
        quad.blend = BLEND_DEFAULT;
        for (int i=0; i<4; ++i) {
            quad.v[i].color = RGBA(0xFF, 0xFF, 0xFF, 0xFF);
            quad.v[i].z = 0.5f;
        }
        quad.v[0].x = gx;            quad.v[0].y = gy;            quad.v[0].tx = glyph->u0; quad.v[0].ty = glyph->v0;
        quad.v[1].x = gx+glyph->w;   quad.v[1].y = gy;            quad.v[1].tx = glyph->u1; quad.v[1].ty = glyph->v0;
        quad.v[2].x = gx+glyph->w;   quad.v[2].y = gy+glyph->h;   quad.v[2].tx = glyph->u1; quad.v[2].ty = glyph->v1;
        quad.v[3].x = gx;            quad.v[3].y = gy+glyph->h;   quad.v[3].tx = glyph->u0; quad.v[3].ty = glyph->v1;
        group->quads.push_back(quad);
    }

    width = pen;
}

void bsglTextMesh::Render(float x, float y, DWORD color, int blend) const {
    for (size_t i=0; i<groups.size(); ++i) {
        const QuadGroup& group = groups[i];
        // retint a copy of the baked quads with the requested color
        std::vector<bsglQuad> quads = group.quads;
        for (size_t j=0; j<quads.size(); ++j) {
            for (int k=0; k<4; ++k) {
                quads[j].v[k].x += x;
                quads[j].v[k].y += y;
                quads[j].v[k].color = color;
                quads[j].blend = blend;
            }
            bsgl->Gfx_RenderQuad(&quads[j]);
        }
    }
}
