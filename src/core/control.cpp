/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: input and control
*/

#include "bsgl_impl.h"

#define _KEY_BIT_MASK 0x3
#define _KEY_DOWN 0x1
#define _KEY_PRESSING 0x3
#define _KEY_UP 0x2

void CALL BSGL_Impl::Control_GetState() {
    Uint8* keystates = SDL_GetKeyState(NULL);
    Uint8 mouse_mask = SDL_GetMouseState(&_mouse_x, &_mouse_y);

    if( keystates[SDLK_a] ) {
        _key_buf[INP_A] = _key_buf[INP_A] << 1 | 1;
    }else {
        _key_buf[INP_A] = _key_buf[INP_A] << 1;
    }

    if( keystates[SDLK_b] ) {
        _key_buf[INP_B] = _key_buf[INP_B] << 1 | 1;
    }else {
        _key_buf[INP_B] = _key_buf[INP_B] << 1;
    }

    if( keystates[SDLK_c] ) {
        _key_buf[INP_C] = _key_buf[INP_C] << 1 | 1;
    }else {
        _key_buf[INP_C] = _key_buf[INP_C] << 1;
    }

    if( keystates[SDLK_d] ) {
        _key_buf[INP_D] = _key_buf[INP_D] << 1 | 1;
    }else {
        _key_buf[INP_D] = _key_buf[INP_D] << 1;
    }

    if( keystates[SDLK_e] ) {
        _key_buf[INP_E] = _key_buf[INP_E] << 1 | 1;
    }else {
        _key_buf[INP_E] = _key_buf[INP_E] << 1;
    }

    if( keystates[SDLK_f] ) {
        _key_buf[INP_F] = _key_buf[INP_F] << 1 | 1;
    }else {
        _key_buf[INP_F] = _key_buf[INP_F] << 1;
    }

    if( keystates[SDLK_g] ) {
        _key_buf[INP_G] = _key_buf[INP_G] << 1 | 1;
    }else {
        _key_buf[INP_G] = _key_buf[INP_G] << 1;
    }

    if( keystates[SDLK_h] ) {
        _key_buf[INP_H] = _key_buf[INP_H] << 1 | 1;
    }else {
        _key_buf[INP_H] = _key_buf[INP_H] << 1;
    }

    if( keystates[SDLK_i] ) {
        _key_buf[INP_I] = _key_buf[INP_I] << 1 | 1;
    }else {
        _key_buf[INP_I] = _key_buf[INP_I] << 1;
    }

    if( keystates[SDLK_j] ) {
        _key_buf[INP_J] = _key_buf[INP_J] << 1 | 1;
    }else {
        _key_buf[INP_J] = _key_buf[INP_J] << 1;
    }

    if( keystates[SDLK_k] ) {
        _key_buf[INP_K] = _key_buf[INP_K] << 1 | 1;
    }else {
        _key_buf[INP_K] = _key_buf[INP_K] << 1;
    }

    if( keystates[SDLK_l] ) {
        _key_buf[INP_L] = _key_buf[INP_L] << 1 | 1;
    }else {
        _key_buf[INP_L] = _key_buf[INP_L] << 1;
    }

    if( keystates[SDLK_m] ) {
        _key_buf[INP_M] = _key_buf[INP_M] << 1 | 1;
    }else {
        _key_buf[INP_M] = _key_buf[INP_M] << 1;
    }

    if( keystates[SDLK_n] ) {
        _key_buf[INP_N] = _key_buf[INP_N] << 1 | 1;
    }else {
        _key_buf[INP_N] = _key_buf[INP_N] << 1;
    }

    if( keystates[SDLK_o] ) {
        _key_buf[INP_O] = _key_buf[INP_O] << 1 | 1;
    }else {
        _key_buf[INP_O] = _key_buf[INP_O] << 1;
    }

    if( keystates[SDLK_p] ) {
        _key_buf[INP_P] = _key_buf[INP_P] << 1 | 1;
    }else {
        _key_buf[INP_P] = _key_buf[INP_P] << 1;
    }

    if( keystates[SDLK_q] ) {
        _key_buf[INP_Q] = _key_buf[INP_Q] << 1 | 1;
    }else {
        _key_buf[INP_Q] = _key_buf[INP_Q] << 1;
    }

    if( keystates[SDLK_r] ) {
        _key_buf[INP_R] = _key_buf[INP_R] << 1 | 1;
    }else {
        _key_buf[INP_R] = _key_buf[INP_R] << 1;
    }

    if( keystates[SDLK_s] ) {
        _key_buf[INP_S] = _key_buf[INP_S] << 1 | 1;
    }else {
        _key_buf[INP_S] = _key_buf[INP_S] << 1;
    }

    if( keystates[SDLK_t] ) {
        _key_buf[INP_T] = _key_buf[INP_T] << 1 | 1;
    }else {
        _key_buf[INP_T] = _key_buf[INP_T] << 1;
    }

    if( keystates[SDLK_u] ) {
        _key_buf[INP_U] = _key_buf[INP_U] << 1 | 1;
    }else {
        _key_buf[INP_U] = _key_buf[INP_U] << 1;
    }

    if( keystates[SDLK_v] ) {
        _key_buf[INP_V] = _key_buf[INP_V] << 1 | 1;
    }else {
        _key_buf[INP_V] = _key_buf[INP_V] << 1;
    }

    if( keystates[SDLK_w] ) {
        _key_buf[INP_W] = _key_buf[INP_W] << 1 | 1;
    }else {
        _key_buf[INP_W] = _key_buf[INP_W] << 1;
    }

    if( keystates[SDLK_x] ) {
        _key_buf[INP_X] = _key_buf[INP_X] << 1 | 1;
    }else {
        _key_buf[INP_X] = _key_buf[INP_X] << 1;
    }

    if( keystates[SDLK_y] ) {
        _key_buf[INP_Y] = _key_buf[INP_Y] << 1 | 1;
    }else {
        _key_buf[INP_Y] = _key_buf[INP_Y] << 1;
    }

    if( keystates[SDLK_z] ) {
        _key_buf[INP_Z] = _key_buf[INP_Z] << 1 | 1;
    }else {
        _key_buf[INP_Z] = _key_buf[INP_Z] << 1;
    }

    if( keystates[SDLK_1] ) {
        _key_buf[INP_1] = _key_buf[INP_1] << 1 | 1;
    }else {
        _key_buf[INP_1] = _key_buf[INP_1] << 1;
    }

    if( keystates[SDLK_2] ) {
        _key_buf[INP_2] = _key_buf[INP_2] << 1 | 1;
    }else {
        _key_buf[INP_2] = _key_buf[INP_2] << 1;
    }

    if( keystates[SDLK_3] ) {
        _key_buf[INP_3] = _key_buf[INP_3] << 1 | 1;
    }else {
        _key_buf[INP_3] = _key_buf[INP_3] << 1;
    }

    if( keystates[SDLK_4] ) {
        _key_buf[INP_4] = _key_buf[INP_4] << 1 | 1;
    }else {
        _key_buf[INP_4] = _key_buf[INP_4] << 1;
    }

    if( keystates[SDLK_5] ) {
        _key_buf[INP_5] = _key_buf[INP_5] << 1 | 1;
    }else {
        _key_buf[INP_5] = _key_buf[INP_5] << 1;
    }

    if( keystates[SDLK_6] ) {
        _key_buf[INP_6] = _key_buf[INP_6] << 1 | 1;
    }else {
        _key_buf[INP_6] = _key_buf[INP_6] << 1;
    }

    if( keystates[SDLK_7] ) {
        _key_buf[INP_7] = _key_buf[INP_7] << 1 | 1;
    }else {
        _key_buf[INP_7] = _key_buf[INP_7] << 1;
    }

    if( keystates[SDLK_8] ) {
        _key_buf[INP_8] = _key_buf[INP_8] << 1 | 1;
    }else {
        _key_buf[INP_8] = _key_buf[INP_8] << 1;
    }

    if( keystates[SDLK_9] ) {
        _key_buf[INP_9] = _key_buf[INP_9] << 1 | 1;
    }else {
        _key_buf[INP_9] = _key_buf[INP_9] << 1;
    }

    if( keystates[SDLK_0] ) {
        _key_buf[INP_0] = _key_buf[INP_0] << 1 | 1;
    }else {
        _key_buf[INP_0] = _key_buf[INP_0] << 1;
    }

    if( keystates[SDLK_ESCAPE] ) {
        _key_buf[INP_ESC] = _key_buf[INP_ESC] << 1 | 1;
    }else {
        _key_buf[INP_ESC] = _key_buf[INP_ESC] << 1;
    }

    if( mouse_mask & SDL_BUTTON(1) ) {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1 | 1;
    }else {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1;
    }

    if( mouse_mask & SDL_BUTTON(2) ) {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1 | 1;
    }else {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1;
    }
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

