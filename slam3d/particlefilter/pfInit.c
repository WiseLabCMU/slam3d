//
//  pfInit.c
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES

#include "pfInit.h"
#include "pfRandom.h"

void pfInit_initTag(tag_t* tag)
{
    int i;
    
    for (i = 0; i < PF_N_TAG; ++i)
        pfInit_spawnTagParticle(&tag->pTag[i]);
}

void pfInit_initBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange)
{
    int i, j;
    const tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &tag->pTag[i];
        for (j = 0; j < PF_N_BCN; ++j)
            pfInit_spawnBcnParticle(&bcn->pBcn[i][j], tp, range, stdRange);
    }
}

void pfInit_spawnTagParticle(tagParticle_t* tp)
{
    tp->w = 1.0f;
    tp->x = 0.0f;
    tp->y = 0.0f;
    tp->z = 0.0f;
    tp->theta = 0.0f;
}

void pfInit_spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange)
{
    int i;
    float rdist, rdistTmp, relev, razim;
    float c, dx, dy, dz;
    
    rdist = 0.0f;
    for (i = 0; i < 10; ++i)
    {
        rdistTmp = range + 3 * stdRange * (pfRandom_uniform() * 2 - 1);
        if (rdistTmp < 0.0f)
            continue;
        rdist = rdistTmp;
        break;
    }
    
    relev = asinf(pfRandom_uniform() * 2 - 1);
    razim = pfRandom_uniform() * 2 * (float)M_PI;
    
    c = rdist * cosf(relev);
    dx = c * cosf(razim);
    dy = c * sinf(razim);
    dz = rdist * sinf(relev);
    
    bp->w = 1.0f;
    bp->x = tp->x + dx;
    bp->y = tp->y + dy;
    bp->z = tp->z + dz;
}
