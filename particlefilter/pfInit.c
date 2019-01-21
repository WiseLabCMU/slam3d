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
    
    pfRandom_init();
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

void pfInit_spawnTagParticleFromOther(tagParticle_t* tp, const tagParticle_t* other, float hXyz, float hTheta)
{
    float dx, dy, dz, dtheta;
    
    pfRandom_normal2(&dx, &dy);
    pfRandom_normal2(&dz, &dtheta);
    tp->w = 1.0f;
    tp->x = other->x + dx * hXyz;
    tp->y = other->y + dy * hXyz;
    tp->z = other->z + dz * hXyz;
    tp->theta = fmodf(other->theta + dtheta * hTheta, 2 * (float)M_PI);
}

void pfInit_spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange)
{
    float dx, dy, dz;
    
    pfRandom_sphere(&dx, &dy, &dz, range, stdRange);
    bp->w = 1.0f;
    bp->x = tp->x + dx;
    bp->y = tp->y + dy;
    bp->z = tp->z + dz;
}

void pfInit_spawnBcnParticleFromOther(bcnParticle_t* bp, const bcnParticle_t* other, float hXyz)
{
    float dx, dy, dz, dtheta;
    
    pfRandom_normal2(&dx, &dy);
    pfRandom_normal2(&dz, &dtheta);
    bp->w = 1.0f;
    bp->x = other->x + dx * hXyz;
    bp->y = other->y + dy * hXyz;
    bp->z = other->z + dz * hXyz;
}
