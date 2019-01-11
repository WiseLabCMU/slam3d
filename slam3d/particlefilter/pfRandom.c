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
    float f = sqrtf(-2 * logf(pfRandom_uniform()));
    float g = pfRandom_uniform() * 2 * (float)M_PI;
    
    *x = f * cosf(g);
    *y = f * sinf(g);
}
