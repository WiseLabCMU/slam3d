/*
 * pfRandom.c
 * Created by John Miller on 11/1/19.
 *
 * Copyright (c) 2019, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES
#include <stdlib.h>
#include <time.h>

#include "particleFilter.h"
#include "pfRandom.h"

#if defined(_WIN32)
#define PF_FORCE_MUSL_RANDR
#endif

#if defined(PF_FORCE_MUSL_RANDR)
#pragma message "Using included MUSL rand_r() implementation (_WIN32 or PF_FORCE_MUSL_RANDR)"
// MUSL rand_r implementation
// https://git.musl-libc.org/cgit/musl/tree/src/prng/rand_r.c?id=20d01d83b5a13c77805976e7c520f566244ba3ff
static unsigned temper(unsigned x)
{
	x ^= x>>11;
	x ^= x<<7 & 0x9D2C5680;
	x ^= x<<15 & 0xEFC60000;
	x ^= x>>18;
	return x;
}

int rand_r_func(unsigned *seed)
{
	return temper(*seed = *seed * 1103515245 + 12345)/2;
}

// https://git.musl-libc.org/cgit/musl/tree/include/stdlib.h?id=821083ac7b54eaa040d5a8ddc67c6206a175e0ca
#define RAND_R_MAX_ACTUAL (0x7fffffff)
#else
#define RAND_R_MAX_ACTUAL RAND_MAX
#define rand_r_func           rand_r
#endif

unsigned int PF_SEED = 0;
int PF_SEED_SET = 0;

static float _uniformNonzero(void);

void pfRandom_init(void)
{
    if (!PF_SEED_SET)
        PF_SEED = (unsigned int)time(NULL);
}

float pfRandom_uniform(void)
{
    return (float)rand_r_func(&PF_SEED) / RAND_R_MAX_ACTUAL;
}

void pfRandom_normal2(float* x, float* y)
{
    float f = sqrtf(-2 * logf(_uniformNonzero()));
    float g = _uniformNonzero() * 2 * (float)M_PI;
    
    *x = f * cosf(g);
    *y = f * sinf(g);
}

void pfRandom_sphere(float* x, float* y, float* z, float range, float stdRange)
{
    int i;
    float rad, radTmp, elev, azim, c;
    
    rad = 0.0f;
    for (i = 0; i < 10; ++i)
    {
        radTmp = range + 3 * stdRange * (pfRandom_uniform() * 2 - 1);
        if (radTmp < 0.0f)
            continue;
        rad = radTmp;
        break;
    }
    
    elev = asinf(pfRandom_uniform() * 2 - 1);
    azim = pfRandom_uniform() * 2 * (float)M_PI;
    
    c = rad * cosf(elev);
    *x = c * cosf(azim);
    *y = c * sinf(azim);
    *z = rad * sinf(elev);
}

static float _uniformNonzero(void)
{
    return (float)(rand_r_func(&PF_SEED) + 1) / ((float)RAND_R_MAX_ACTUAL + 1);
}
