/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: timer
*/

#include "bsgl_impl.h"

float CALL BSGL_Impl::Timer_GetTime() {
    return fTime;
}

float CALL BSGL_Impl::Timer_GetDelta() {
    return fDeltaTime;
}

int CALL BSGL_Impl::Timer_GetFPS() {
    return nFPS;
}

