/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: input and control
*/

#include "bsgl_impl.h"

#include "ScreenWidget.h"

#define _KEY_BIT_MASK 0x3
#define _KEY_DOWN 0x1
#define _KEY_PRESSING 0x3
#define _KEY_UP 0x2

bool qtKeyStates[0xFF];
bool qtLeftButton;
bool qtRightButton;

#define PROC_KEY(k)                                     \
    if (qtKeyStates[Qt::Key_##k]) {                     \
        _key_buf[INP_##k] = _key_buf[INP_##k] << 1 | 1; \
    }else {                                             \
        _key_buf[INP_##k] = _key_buf[INP_##k] << 1;     \
    }

void CALL BSGL_Impl::Control_GetState() {
    ScreenWidget* sw = ScreenWidget::instance();
    QPoint mousePos = sw->mapFromGlobal(sw->cursor().pos());
    _mouse_x = mousePos.x();
    _mouse_y = mousePos.y();

    if (qtLeftButton) {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1 | 1;
    }else {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1;
    }

    if (qtRightButton) {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1 | 1;
    }else {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1;
    }

    PROC_KEY(A);
    PROC_KEY(B);
    PROC_KEY(C);
    PROC_KEY(D);
    PROC_KEY(E);
    PROC_KEY(F);
    PROC_KEY(G);
    PROC_KEY(H);
    PROC_KEY(I);
    PROC_KEY(J);
    PROC_KEY(K);
    PROC_KEY(L);
    PROC_KEY(M);
    PROC_KEY(N);
    PROC_KEY(O);
    PROC_KEY(P);
    PROC_KEY(Q);
    PROC_KEY(R);
    PROC_KEY(S);
    PROC_KEY(T);
    PROC_KEY(U);
    PROC_KEY(V);
    PROC_KEY(W);
    PROC_KEY(X);
    PROC_KEY(Y);
    PROC_KEY(Z);
    PROC_KEY(0);
    PROC_KEY(1);
    PROC_KEY(2);
    PROC_KEY(3);
    PROC_KEY(4);
    PROC_KEY(5);
    PROC_KEY(6);
    PROC_KEY(7);
    PROC_KEY(8);
    PROC_KEY(9);
}

bool CALL BSGL_Impl::Control_IsDown(int key) {
    if( (_key_buf[key]&_KEY_BIT_MASK) == _KEY_DOWN ) {
        return true;
    }else {
        return false;
    }
}

bool CALL BSGL_Impl::Control_IsPassing(int key) {
    if( (_key_buf[key]&_KEY_BIT_MASK) == _KEY_PRESSING ) {
        return true;
    }else {
        return false;
    }
}

bool CALL BSGL_Impl::Control_IsUp(int key) {
    if( (_key_buf[key]&_KEY_BIT_MASK) == _KEY_UP ) {
        return true;
    }else {
        return false;
    }
}

int  CALL BSGL_Impl::Control_GetMouseX() {
    return _mouse_x;
}

int  CALL BSGL_Impl::Control_GetMouseY() {
    return _mouse_y;
}

