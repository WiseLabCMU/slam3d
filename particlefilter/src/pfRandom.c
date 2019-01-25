//
//  pfRandom.c
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES
#include <stdlib.h>
#include <time.h>

#include "pfRandom.h"

static float _uniformNonzero(void);

void pfRandom_init(void)
{
    srand((unsigned int)time(NULL));
}

float pfRandom_uniform(void)
{
    return (float)rand() / RAND_MAX;
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
    return (float)(rand() + 1) / (RAND_MAX + 1);
}
