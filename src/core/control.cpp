/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** Core functions implementation: input and control (SDL3 frontend)
*/

#include "bsgl_impl.h"

#define _KEY_BIT_MASK 0x3
#define _KEY_DOWN 0x1
#define _KEY_PRESSING 0x3
#define _KEY_UP 0x2

struct _KeyMap {
    int             inp;
    SDL_Scancode    scancode;
};

static const _KeyMap _key_map[] = {
    { INP_A,        SDL_SCANCODE_A          },
    { INP_B,        SDL_SCANCODE_B          },
    { INP_C,        SDL_SCANCODE_C          },
    { INP_D,        SDL_SCANCODE_D          },
    { INP_E,        SDL_SCANCODE_E          },
    { INP_F,        SDL_SCANCODE_F          },
    { INP_G,        SDL_SCANCODE_G          },
    { INP_H,        SDL_SCANCODE_H          },
    { INP_I,        SDL_SCANCODE_I          },
    { INP_J,        SDL_SCANCODE_J          },
    { INP_K,        SDL_SCANCODE_K          },
    { INP_L,        SDL_SCANCODE_L          },
    { INP_M,        SDL_SCANCODE_M          },
    { INP_N,        SDL_SCANCODE_N          },
    { INP_O,        SDL_SCANCODE_O          },
    { INP_P,        SDL_SCANCODE_P          },
    { INP_Q,        SDL_SCANCODE_Q          },
    { INP_R,        SDL_SCANCODE_R          },
    { INP_S,        SDL_SCANCODE_S          },
    { INP_T,        SDL_SCANCODE_T          },
    { INP_U,        SDL_SCANCODE_U          },
    { INP_V,        SDL_SCANCODE_V          },
    { INP_W,        SDL_SCANCODE_W          },
    { INP_X,        SDL_SCANCODE_X          },
    { INP_Y,        SDL_SCANCODE_Y          },
    { INP_Z,        SDL_SCANCODE_Z          },
    { INP_1,        SDL_SCANCODE_1          },
    { INP_2,        SDL_SCANCODE_2          },
    { INP_3,        SDL_SCANCODE_3          },
    { INP_4,        SDL_SCANCODE_4          },
    { INP_5,        SDL_SCANCODE_5          },
    { INP_6,        SDL_SCANCODE_6          },
    { INP_7,        SDL_SCANCODE_7          },
    { INP_8,        SDL_SCANCODE_8          },
    { INP_9,        SDL_SCANCODE_9          },
    { INP_0,        SDL_SCANCODE_0          },
    { INP_F1,       SDL_SCANCODE_F1         },
    { INP_F2,       SDL_SCANCODE_F2         },
    { INP_F3,       SDL_SCANCODE_F3         },
    { INP_F4,       SDL_SCANCODE_F4         },
    { INP_F5,       SDL_SCANCODE_F5         },
    { INP_F6,       SDL_SCANCODE_F6         },
    { INP_F7,       SDL_SCANCODE_F7         },
    { INP_F8,       SDL_SCANCODE_F8         },
    { INP_F9,       SDL_SCANCODE_F9         },
    { INP_F10,      SDL_SCANCODE_F10        },
    { INP_F11,      SDL_SCANCODE_F11        },
    { INP_F12,      SDL_SCANCODE_F12        },
    { INP_ESC,      SDL_SCANCODE_ESCAPE     },
    { INP_TAB,      SDL_SCANCODE_TAB        },
    { INP_CAPSLOCK, SDL_SCANCODE_CAPSLOCK   },
    { INP_SHIFTL,   SDL_SCANCODE_LSHIFT     },
    { INP_SHIFTR,   SDL_SCANCODE_RSHIFT     },
    { INP_CTRLL,    SDL_SCANCODE_LCTRL      },
    { INP_CTRLR,    SDL_SCANCODE_RCTRL      },
    { INP_ALTL,     SDL_SCANCODE_LALT       },
    { INP_ALTR,     SDL_SCANCODE_RALT       },
    { INP_SPACE,    SDL_SCANCODE_SPACE      },
    { INP_ENTER,    SDL_SCANCODE_RETURN     },
    { INP_DEL,      SDL_SCANCODE_DELETE     },
    { INP_UP,       SDL_SCANCODE_UP         },
    { INP_DOWN,     SDL_SCANCODE_DOWN       },
    { INP_LEFT,     SDL_SCANCODE_LEFT       },
    { INP_RIGHT,    SDL_SCANCODE_RIGHT      },
    { INP_HOME,     SDL_SCANCODE_HOME       },
    { INP_END,      SDL_SCANCODE_END        },
    { INP_PGUP,     SDL_SCANCODE_PAGEUP     },
    { INP_PGDN,     SDL_SCANCODE_PAGEDOWN   },
};

void CALL BSGL_Impl::Control_GetState() {
    float mx;
    float my;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    _mouse_x = (int)mx;
    _mouse_y = (int)my;

    if (buttons & SDL_BUTTON_LMASK) {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1 | 1;
    }else {
        _key_buf[INP_MOUSEL] = _key_buf[INP_MOUSEL] << 1;
    }

    if (buttons & SDL_BUTTON_RMASK) {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1 | 1;
    }else {
        _key_buf[INP_MOUSER] = _key_buf[INP_MOUSER] << 1;
    }

    bool const* keys = SDL_GetKeyboardState(0);
    int const num_keys = sizeof(_key_map)/sizeof(_key_map[0]);
    for (int i=0; i<num_keys; ++i) {
        if (keys[_key_map[i].scancode]) {
            _key_buf[_key_map[i].inp] = _key_buf[_key_map[i].inp] << 1 | 1;
        }else {
            _key_buf[_key_map[i].inp] = _key_buf[_key_map[i].inp] << 1;
        }
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
