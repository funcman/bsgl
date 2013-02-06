/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglRect util class header
*/

#ifndef BSGLRECT_H
#define BSGLRECT_H

class bsglRect {
public:
    float x1;
    float y1;
    float x2;
    float y2;

    bsglRect();
    bsglRect(float _x1, float _y1, float _x2, float _y2);

    void Clear();
    bool IsClean();
    void Set(float _x1, float _y1, float _x2, float _y2);
    void SetRadius(float x, float y, float r);
    void Encapsulate(float x, float y);
    bool TestPoint(float x, float y) const;
    bool Intersect(bsglRect const* rect) const;

private:
    bool bClean;
};

#endif//BSGLRECT_H
