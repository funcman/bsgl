/*
 ** Buggy-Mushroom's Spore Game Library
 ** Copyright (C) 2008-2015, Buggy-Mushroom Studio
 **
 ** bsglWidget class implementation
 */

#include "bsglwidget.h"
#include <string.h>

BSGL* bsglWidget::bsgl=0;

bsglWidget::bsglWidget(int x, int y, int width, int height) {
    bsgl = bsglCreate(BSGL_VERSION);
    x_ = x;
    y_ = y;
    w_ = width;
    h_ = height;
    col_ = RGBA(255, 255, 255, 255);
    focus_ = false;
    quad_.v[0].z = 0.f;
    quad_.v[1].z = 0.f;
    quad_.v[2].z = 0.f;
    quad_.v[3].z = 0.f;
    quad_.tex = 0;
    quad_.blend = BLEND_DEFAULT;
}

void bsglWidget::SetX(int x) {
    x_ = x;
}

void bsglWidget::SetY(int y) {
    y_ = y;
}

void bsglWidget::SetBackgroundColor(DWORD color) {
    col_ = color;
}

void bsglWidget::AddKid(bsglWidget* kid) {
    kids_.push_back(kid);
}

void bsglWidget::Render(float x, float y) {
    this->OnRender(x, y);

    std::list<bsglWidget*>::iterator itr = kids_.begin();
    for (; itr!=kids_.end(); ++itr) {
        bsglWidget* w = *itr;
        w->Render(x_+x, y_+y);
    }
}

void bsglWidget::MouseAt(float x, float y, MouseState state) {
    static float ox;
    static float oy;
    bool post = false;
    switch (state) {
        case MouseState_Default: {
            this->OnOver(x, y);
            post = true;
        }break;
        case MouseState_Down: {
            if (TestAt(x, y)) {
                bool inKids = false;
                std::list<bsglWidget*>::iterator itr = kids_.begin();
                bsglWidget* w;
                for (; itr!=kids_.end(); ++itr) {
                    w = *itr;
                    if (w->TestAt(x-x_, y-y_)) {
                        inKids = true;
                        break;
                    }
                }
                if (inKids) {
                    focus_ = false;
                    w->MouseAt(x-x_, y-y_, state);
                }else {
                    focus_ = true;
                    ox = x;
                    oy = y;
                    this->OnDown();
                }
            }
            post = false;
        }break;
        case MouseState_Passing: {
            if (focus_) {
                this->OnMove(x-ox, y-ox);
                ox = x;
                ox = y;
            }
            post = true;
        }break;
        case MouseState_Up: {
            if (focus_) {
                this->OnUp(TestAt(x, y));
                focus_ = false;
            }
            post = true;
        }break;
        default: break;
    }
    if (post) {
        std::list<bsglWidget*>::iterator itr = kids_.begin();
        for (; itr!=kids_.end(); ++itr) {
            bsglWidget* w = *itr;
            w->MouseAt(x-x_, y-y_, state);
        }
    }
}

bool bsglWidget::TestAt(float x, float y) {
    if (x >= x_ && x < x_+w_ && y >= y_ && y < y_+h_) {
        return true;
    }
    return false;
}

void bsglWidget::OnRender(float x, float y) {
    quad_.v[0].x = x_+x;    quad_.v[0].y = y_+y;
    quad_.v[1].x = x_+x+w_; quad_.v[1].y = y_+y;
    quad_.v[2].x = x_+x+w_; quad_.v[2].y = y_+y+h_;
    quad_.v[3].x = x_+x;    quad_.v[3].y = y_+y+h_;
    quad_.v[0].color = col_;
    quad_.v[1].color = col_;
    quad_.v[2].color = col_;
    quad_.v[3].color = col_;
    bsgl->Gfx_RenderQuad(&quad_);
}

void bsglWidget::OnOver(float x, float y) {
    return;
}

void bsglWidget::OnDown() {
    return;
}

void bsglWidget::OnMove(float dx, float dy) {
    return;
}

void bsglWidget::OnUp(bool inside) {
    return;
}
