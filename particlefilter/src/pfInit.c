/*
 * pfInit.c
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

#include "pfInit.h"
#include "pfRandom.h"

void pfInit_initTagLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
{
    int i;
    for (i = 0; i < PF_N_TAG_LOC; ++i)
        pfInit_spawnTagParticleFromRange(&pf->pTag[i], bx, by, bz, range, stdRange);
}

void pfInit_initTagSlam(particleFilterSlam_t* pf)
{
    int i;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
        pfInit_spawnTagParticleZero(&pf->pTag[i]);
}

void pfInit_initBcnSlam(bcn_t* bcn, const particleFilterSlam_t* pf, float range, float stdRange)
{
    int i, j;
    const tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        tp = &pf->pTag[i];
        for (j = 0; j < PF_N_BCN; ++j)
            pfInit_spawnBcnParticleFromRange(&bcn->pBcn[i][j], tp, range, stdRange);
    }
}

void pfInit_spawnTagParticleZero(tagParticle_t* tp)
{
    tp->w = 1.0f;
    tp->x = 0.0f;
    tp->y = 0.0f;
    tp->z = 0.0f;
    tp->theta = 0.0f;
}

void pfInit_spawnTagParticleFromRange(tagParticle_t* tp, float bx, float by, float bz, float range, float stdRange)
{
    float dx, dy, dz;

    pfRandom_sphere(&dx, &dy, &dz, range, stdRange);
    tp->w = 1.0f;
    tp->x = bx + dx;
    tp->y = by + dy;
    tp->z = bz + dz;
    tp->theta = pfRandom_uniform() * 2 * (float)M_PI;
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

void pfInit_spawnBcnParticleFromRange(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange)
{
    float dx, dy, dz;
    
    pfRandom_sphere(&dx, &dy, &dz, range, stdRange);
    bp->w = 1.0f;
    bp->x = tp->x + dx;
    bp->y = tp->y + dy;
    bp->z = tp->z + dz;
    bp->theta = pfRandom_uniform() * 2 * (float)M_PI;
}

void pfInit_spawnBcnParticleFromOther(bcnParticle_t* bp, const bcnParticle_t* other, float hXyz, float hTheta)
{
    float dx, dy, dz, dtheta;
    
    pfRandom_normal2(&dx, &dy);
    pfRandom_normal2(&dz, &dtheta);
    bp->w = 1.0f;
    bp->x = other->x + dx * hXyz;
    bp->y = other->y + dy * hXyz;
    bp->z = other->z + dz * hXyz;
    bp->theta = fmodf(other->theta + dtheta * hTheta, 2 * (float)M_PI);
}
