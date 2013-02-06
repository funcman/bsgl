/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008-2009, Buggy-Mushroom Studio
**
** bsglRect util class implementation
*/

#include "bsglrect.h"
#include <math.h>

bsglRect::bsglRect() {
    bClean = true;
}

bsglRect::bsglRect(float _x1, float _y1, float _x2, float _y2) {
    x1 = _x1;
    y1 = _y1;
    x2 = _x2;
    y2 = _y2;
    bClean = false;
}

void bsglRect::Clear() {
    bClean = true;
}

bool bsglRect::IsClean() {
    return bClean;
}

void bsglRect::Set(float _x1, float _y1, float _x2, float _y2) {
    x1 = _x1;
    y1 = _y1;
    x2 = _x2;
    y2 = _y2;
    bClean = false;
}

void bsglRect::SetRadius(float x, float y, float r) {
    x1 = x - r;
    y1 = y - r;
    x2 = x + r;
    y2 = y + r;
    bClean = false;
}

void bsglRect::Encapsulate(float x, float y) {
    if( bClean ) {
        x1=x2=x;
        y1=y2=y;
        bClean=false;
    }else {
        if(x<x1) x1=x;
        if(x>x2) x2=x;
        if(y<y1) y1=y;
        if(y>y2) y2=y;
    }
}

bool bsglRect::TestPoint(float x, float y) const {
    if( x>=x1 && x<x2 && y>=y1 && y<y2 ) {
        return true;
    }
    return false;
}

bool bsglRect::Intersect(bsglRect const* rect) const {
    if( fabs(x1+x2-rect->x1-rect->x2) < (x2-x1+rect->x2-rect->x1) ) {
        if( fabs(y1+y2-rect->y1-rect->y2) < (y2-y1+rect->y2-rect->y1) ) {
            return true;
        }
    }
    return false;
}
