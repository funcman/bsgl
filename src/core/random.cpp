/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: random number generation
*/

#include "bsgl_impl.h"

static unsigned int g_seed = 0;

void CALL BSGL_Impl::Random_Seed(int seed) {
    if( 0 == seed ) {
        g_seed = QTime().elapsed();
    }else {
        g_seed = seed;
    }
}

int CALL BSGL_Impl::Random_Int(int min, int max) {
    g_seed = 214013 * g_seed + 2531011;
    return min + (g_seed ^ g_seed >> 15) % (max - min + 1);
}

float CALL BSGL_Impl::Random_Float(float min, float max) {
    g_seed = 214013 * g_seed + 2531011;
    return min + (g_seed >> 16) * (1.0f/65535.0f) * (max - min);
}
