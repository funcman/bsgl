/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** bsglGUI control classes implementation (Label / Button / Slider)
*/

#include "bsglguictrls.h"
#include <string.h>

//=====================================================================
// bsglGUILabel
//=====================================================================

bsglGUILabel::bsglGUILabel(int x, int y, int w, int h, bsglFont* font)
    : bsglGUIWidget(x, y, w, h) {
    font_   = font;
    align_  = BSGLGUI_LEFT;
    mesh_   = new bsglTextMesh();
}

bsglGUILabel::~bsglGUILabel() {
    delete mesh_;
}

void bsglGUILabel::SetMode(int align) {
    align_ = align;
}

void bsglGUILabel::SetText(char const* text) {
    mesh_->Build(font_, text);
}

void bsglGUILabel::OnRender(float x, float y) {
    if (mesh_->IsEmpty()) {
        return;
    }

    float tw = mesh_->GetWidth();
    float th = mesh_->GetHeight();
    if (th <= 0.0f) {
        th = font_->GetLineHeight();
    }
    float ty = y_ + y + (h_ - th) / 2.0f;

    float tx;
    switch (align_) {
        case BSGLGUI_RIGHT:     tx = (float)(x_ + x + w_) - tw; break;
        case BSGLGUI_CENTER:    tx = x_ + x + (w_ - tw) / 2.0f;  break;
        case BSGLGUI_LEFT:
        default:                tx = (float)(x_ + x);            break;
    }

    mesh_->Render(tx, ty, col_);
}

//=====================================================================
// bsglGUIButton
//=====================================================================

bsglGUIButton::bsglGUIButton(int x, int y, int w, int h,
                             HTEXTURE tex,
                             float tx_up, float ty_up,
                             float tx_down, float ty_down)
    : bsglGUIWidget(x, y, w, h) {
    bTrigger = false;
    bPressed = false;
    bOldState = false;
    sprUp     = new bsglSprite(tex, tx_up, ty_up, (float)w, (float)h);
    sprDown   = new bsglSprite(tex, tx_down, ty_down, (float)w, (float)h);
}

bsglGUIButton::~bsglGUIButton() {
    delete sprUp;
    delete sprDown;
}

void bsglGUIButton::SetMode(bool bTrigger) {
    this->bTrigger = bTrigger;
}

void bsglGUIButton::SetState(bool bDown) {
    bPressed = bDown;
}

void bsglGUIButton::OnRender(float x, float y) {
    if (bPressed) {
        sprDown->Render((float)(x_+x), (float)(y_+y));
    } else {
        sprUp->Render((float)(x_+x), (float)(y_+y));
    }
}

void bsglGUIButton::OnDown() {
    bOldState = bPressed;
    bPressed = true;
}

void bsglGUIButton::OnUp(bool inside) {
    if (bTrigger) {
        if (inside) {
            bPressed = !bOldState;
        }
    } else {
        bPressed = false;
    }
    if (inside) {
        OnClick();
    }
}

void bsglGUIButton::OnClick() {
}

//=====================================================================
// bsglGUISlider
//=====================================================================

bsglGUISlider::bsglGUISlider(int x, int y, int w, int h,
                             HTEXTURE tex, float tx, float ty,
                             float sw, float sh, bool vertical)
    : bsglGUIWidget(x, y, w, h) {
    bPressed  = false;
    bVertical = vertical;
    mode      = BSGLSLIDER_BAR;
    fMin      = 0.0f;
    fMax      = 100.0f;
    fVal      = 50.0f;
    sl_w      = sw;
    sl_h      = sh;
    lastX     = 0.0f;
    lastY     = 0.0f;
    sprSlider = new bsglSprite(tex, tx, ty, sw, sh);
}

bsglGUISlider::~bsglGUISlider() {
    delete sprSlider;
}

void bsglGUISlider::SetMode(float fMin, float fMax, int mode) {
    this->fMin = fMin;
    this->fMax = fMax;
    this->mode = mode;
}

void bsglGUISlider::SetValue(float val) {
    if (val < fMin) val = fMin;
    if (val > fMax) val = fMax;
    fVal = val;
}

void bsglGUISlider::OnRender(float x, float y) {
    float x1, y1, x2, y2;
    float xx = x_ + x + (x_ + x + w_ - (x_ + x)) * (fVal - fMin) / (fMax - fMin);
    float yy = y_ + y + (y_ + y + h_ - (y_ + y)) * (fVal - fMin) / (fMax - fMin);

    if (!bVertical) {
        switch (mode) {
            case BSGLSLIDER_BAR:
                x1 = (float)(x_+x); y1 = (float)(y_+y);
                x2 = xx;            y2 = (float)(y_+y+h_);
                break;
            case BSGLSLIDER_BARRELATIVE:
                x1 = x_ + x + w_/2.0f;  y1 = (float)(y_+y);
                x2 = xx;                y2 = (float)(y_+y+h_);
                break;
            case BSGLSLIDER_SLIDER:
            default:
                x1 = xx - sl_w/2.0f;    y1 = y_ + y + (h_ - sl_h)/2.0f;
                x2 = xx + sl_w/2.0f;    y2 = y1 + sl_h;
                break;
        }
    } else {
        switch (mode) {
            case BSGLSLIDER_BAR:
                x1 = (float)(x_+x); y1 = (float)(y_+y);
                x2 = (float)(x_+x+w_);  y2 = yy;
                break;
            case BSGLSLIDER_BARRELATIVE:
                x1 = (float)(x_+x);     y1 = y_ + y + h_/2.0f;
                x2 = (float)(x_+x+w_);  y2 = yy;
                break;
            case BSGLSLIDER_SLIDER:
            default:
                x1 = x_ + x + (w_ - sl_w)/2.0f;    y1 = yy - sl_h/2.0f;
                x2 = x1 + sl_w;                    y2 = yy + sl_h/2.0f;
                break;
        }
    }
    sprSlider->RenderStretch(x1, y1, x2, y2);
}

void bsglGUISlider::OnOver(float x, float y) {
    // x/y are in this widget's parent coordinate space; store local coords
    lastX = x - x_;
    lastY = y - y_;
}

void bsglGUISlider::OnDown() {
    bPressed = true;
    // jump to the press position immediately
    OnMove(0.0f, 0.0f);
}

void bsglGUISlider::OnMove(float dx, float dy) {
    if (!bPressed) {
        return;
    }
    lastX += dx;
    lastY += dy;

    float pos, len;
    if (!bVertical) {
        pos = lastX;
        len = (float)w_;
    } else {
        pos = lastY;
        len = (float)h_;
    }
    if (pos < 0.0f)      pos = 0.0f;
    if (pos > len)       pos = len;

    fVal = fMin + (fMax - fMin) * pos / len;
}

void bsglGUISlider::OnUp(bool inside) {
    bPressed = false;
}
