/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (C) 2008-2015, Buggy-Mushroom Studio
 **
 ** bsglWidget class header
 */

#ifndef BSGLWIDGET_H
#define BSGLWIDGET_H

#include "bsgl.h"
#include <list>

typedef enum {
    MouseState_Default,
    MouseState_Down,
    MouseState_Passing,
    MouseState_Up,
} MouseState;

class bsglWidget {
public:
    bsglWidget(int x, int y, int width, int height);
    ~bsglWidget() { bsgl->Release(); }

    void SetX(int x);
    void SetY(int y);

    void SetBackgroundColor(DWORD color);
    void AddKid(bsglWidget* kid);

    void Render(float x, float y);

    void MouseAt(float x, float y, MouseState state);

    bool TestAt(float x, float y);

    virtual void OnRender(float x, float y);
    virtual void OnOver(float x, float y);
    virtual void OnDown();
    virtual void OnMove(float dx, float dy);
    virtual void OnUp(bool inside);

protected:
    bsglWidget();
    static BSGL* bsgl;

    int x_, y_;
    int w_, h_;
    DWORD col_;

    bool focus_;

    std::list<bsglWidget*> kids_;

    bsglQuad quad_;
};

#endif//BSGLWIDGET_H
