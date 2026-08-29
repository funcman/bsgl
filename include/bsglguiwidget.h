/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (c) 2008-2026 Buggy-Mushroom Studio
 **
 ** bsglGUIWidget class header
 */

#ifndef BSGLGUIWIDGET_H
#define BSGLGUIWIDGET_H

#include "bsgl.h"
#include <list>

typedef enum {
    MouseState_Default,
    MouseState_Down,
    MouseState_Passing,
    MouseState_Up,
} MouseState;

class bsglGUIWidget {
public:
    bsglGUIWidget(int x, int y, int width, int height);
    ~bsglGUIWidget() { bsgl->Release(); }

    void SetX(int x);
    void SetY(int y);

    void SetBackgroundColor(DWORD color);
    void AddKid(bsglGUIWidget* kid);

    void Render(float x, float y);

    void MouseAt(float x, float y, MouseState state);

    bool TestAt(float x, float y);

    virtual void OnRender(float x, float y);
    virtual void OnOver(float x, float y);
    virtual void OnDown();
    virtual void OnMove(float dx, float dy);
    virtual void OnUp(bool inside);

protected:
    bsglGUIWidget();
    static BSGL* bsgl;

    int x_, y_;
    int w_, h_;
    DWORD col_;

    bool focus_;

    std::list<bsglGUIWidget*> kids_;

    bsglQuad quad_;
};

#endif//BSGLGUIWIDGET_H
