/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglGUI control classes header (Label / Button / Slider),
** modeled after HGE's hgeGUIText / hgeGUIButton / hgeGUISlider
*/

#ifndef BSGLGUICTRLS_H
#define BSGLGUICTRLS_H

#include "bsglguiwidget.h"
#include "bsglfont.h"
#include "bsgltextmesh.h"
#include "bsglsprite.h"

#define BSGLGUI_LEFT     0
#define BSGLGUI_CENTER   1
#define BSGLGUI_RIGHT    2

#define BSGLSLIDER_BAR           0
#define BSGLSLIDER_BARRELATIVE   1
#define BSGLSLIDER_SLIDER        2

/*
** A static text label. The widget rect vertically centers the text;
** SetMode() picks the horizontal alignment inside the rect.
** The color set through SetBackgroundColor() tints the text.
*/
class bsglGUILabel : public bsglGUIWidget {
public:
    bsglGUILabel(int x, int y, int w, int h, bsglFont* font);
    ~bsglGUILabel();

    void SetMode(int align);
    void SetText(char const* text);

    int GetWidth() const    { return w_; }
    int GetHeight() const   { return h_; }

    virtual void OnRender(float x, float y);

protected:
    bsglFont*        font_;
    bsglTextMesh*    mesh_;
    int              align_;
};

/*
** A push button with separate up/down sprites. In trigger mode the
** pressed/released state is toggled and kept, like HGE's hgeGUIButton.
*/
class bsglGUIButton : public bsglGUIWidget {
public:
    bsglGUIButton(int x, int y, int w, int h,
                  HTEXTURE tex,
                  float tx_up, float ty_up,
                  float tx_down, float ty_down);
    ~bsglGUIButton();

    void SetMode(bool bTrigger);
    void SetState(bool bDown);
    bool GetState() const { return bPressed; }

    virtual void OnRender(float x, float y);
    virtual void OnDown();
    virtual void OnUp(bool inside);
    virtual void OnClick();

protected:
    bool         bTrigger;
    bool         bPressed;
    bool         bOldState;
    bsglSprite*  sprUp;
    bsglSprite*  sprDown;
};

/*
** A slider bar. Renders in BAR / BARRELATIVE / SLIDER modes like
** HGE's hgeGUISlider; dragging maps the mouse position onto
** [fMin, fMax].
*/
class bsglGUISlider : public bsglGUIWidget {
public:
    bsglGUISlider(int x, int y, int w, int h,
                  HTEXTURE tex, float tx, float ty,
                  float sw, float sh, bool vertical=false);
    ~bsglGUISlider();

    void SetMode(float fMin, float fMax, int mode);
    void SetValue(float fVal);
    float GetValue() const { return fVal; }

    virtual void OnRender(float x, float y);
    virtual void OnOver(float x, float y);
    virtual void OnDown();
    virtual void OnMove(float dx, float dy);
    virtual void OnUp(bool inside);

protected:
    bool         bPressed;
    bool         bVertical;
    int          mode;
    float        fMin, fMax, fVal;
    float        sl_w, sl_h;
    float        lastX, lastY;  // local mouse position, updated while over/dragging
    bsglSprite*  sprSlider;
};

#endif//BSGLGUICTRLS_H
