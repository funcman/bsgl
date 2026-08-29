/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglTextMesh class header
*/

#ifndef BSGLTEXTMESH_H
#define BSGLTEXTMESH_H

#include "bsgl.h"
#include <vector>

class bsglFont;

/*
** A reusable mesh of quads that render a text string through a
** bsglFont glyph atlas. Layout happens once in Build(); Render()
** just re-submits the quads (optionally offset and tinted), so it is
** well suited for GUI labels that redraw the same text every frame.
** Quads are grouped per atlas page because a string may be spread
** over several pages.
*/
class bsglTextMesh {
public:
    bsglTextMesh();

    void Build(bsglFont* font, char const* text);
    void Render(float x, float y, DWORD color=0xFFFFFFFF,
                int blend=BLEND_DEFAULT) const;

    float GetWidth() const   { return width; }
    float GetHeight() const  { return height; }
    bool  IsEmpty() const    { return groups.empty(); }

protected:
    static BSGL* bsgl;

    struct QuadGroup {
        HTEXTURE            tex;
        int                 blend;
        std::vector<bsglQuad> quads;
    };

    std::vector<QuadGroup> groups;
    float   width;
    float   height;
};

#endif//BSGLTEXTMESH_H
