/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: timer
*/

#include "bsgl_impl.h"

float CALL BSGL_Impl::Timer_GetTime() {
#if !defined(CC_TARGET_OS_IPHONE)
    return fTime;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec  + tv.tv_usec / 1000000.0f;
#endif
}

float CALL BSGL_Impl::Timer_GetDelta() {
    return fDeltaTime;
}

int CALL BSGL_Impl::Timer_GetFPS() {
    return nFPS;
}

