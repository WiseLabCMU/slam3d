/*
 * pfMeasurement.c
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

#include "pfMeasurement.h"
#include "pfRandom.h"

#define VIO_STD_XYZ         (1e-3f)
#define VIO_STD_THETA       (1e-6f)
#define MIN_WEIGHT(range)   ((range < 3.0f) ? 0.1f : 0.5f)

void pfMeasurement_applyVioLoc(particleFilterLoc_t* pf, float dt, float dx, float dy, float dz, float ddist)
{
    int i;
    tagParticle_t* tp;
    float c, s, pDx, pDy, stdXyz, stdTheta;
    float rx, ry, rz, rtheta;
    
    stdXyz = sqrtf(ddist) * VIO_STD_XYZ;
    stdTheta = sqrtf(dt) * VIO_STD_THETA;
    for (i = 0; i < PF_N_TAG_LOC; ++i)
    {
        tp = &pf->pTag[i];
        c = cosf(tp->theta);
        s = sinf(tp->theta);
        pDx = dx * c - dy * s;
        pDy = dx * s + dy * c;
        
        pfRandom_normal2(&rx, &ry);
        pfRandom_normal2(&rz, &rtheta);
        
        tp->x += pDx + stdXyz * rx;
        tp->y += pDy + stdXyz * ry;
        tp->z += dz + stdXyz * rz;
        tp->theta = fmodf(tp->theta + stdTheta * rtheta, 2 * (float)M_PI);
    }
}

void pfMeasurement_applyTagVioSlam(particleFilterSlam_t* pf, float dt, float dx, float dy, float dz, float ddist)
{
    int i;
    tagParticle_t* tp;
    float c, s, pDx, pDy, stdXyz, stdTheta;
    float rx, ry, rz, rtheta;

    stdXyz = sqrtf(ddist) * VIO_STD_XYZ;
    stdTheta = sqrtf(dt) * VIO_STD_THETA;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        tp = &pf->pTag[i];
        c = cosf(tp->theta);
        s = sinf(tp->theta);
        pDx = dx * c - dy * s;
        pDy = dx * s + dy * c;

        pfRandom_normal2(&rx, &ry);
        pfRandom_normal2(&rz, &rtheta);

        tp->x += pDx + stdXyz * rx;
        tp->y += pDy + stdXyz * ry;
        tp->z += dz + stdXyz * rz;
        tp->theta = fmodf(tp->theta + stdTheta * rtheta, 2 * (float)M_PI);
    }
}

void pfMeasurement_applyBcnVioSlam(bcn_t* bcn, float dt, float dx, float dy, float dz, float ddist)
{
    int i, j;
    bcnParticle_t* bp;
    float c, s, pDx, pDy, stdXyz, stdTheta;
    float rx, ry, rz, rtheta;

    stdXyz = sqrtf(ddist) * VIO_STD_XYZ;
    stdTheta = sqrtf(dt) * VIO_STD_THETA;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        for (j = 0; j < PF_N_BCN; ++j)
        {
            bp = &bcn->pBcn[i][j];
            c = cosf(bp->theta);
            s = sinf(bp->theta);
            pDx = dx * c - dy * s;
            pDy = dx * s + dy * c;

            pfRandom_normal2(&rx, &ry);
            pfRandom_normal2(&rz, &rtheta);

            bp->x += pDx + stdXyz * rx;
            bp->y += pDy + stdXyz * ry;
            bp->z += dz + stdXyz * rz;
            bp->theta = fmodf(bp->theta + stdTheta * rtheta, 2 * (float)M_PI);
        }
    }
}

void pfMeasurement_applyRangeLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
{
    int i;
    tagParticle_t* tp;
    float minWeight, dx, dy, dz, pRange;

    minWeight = MIN_WEIGHT(range);
    for (i = 0; i < PF_N_TAG_LOC; ++i)
    {
        tp = &pf->pTag[i];
        dx = tp->x - bx;
        dy = tp->y - by;
        dz = tp->z - bz;
        pRange = sqrtf(dx * dx + dy * dy + dz * dz);
        if (fabsf(pRange - range) > 3 * stdRange)
            tp->w *= minWeight;
    }
}

void pfMeasurement_applyRangeSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    bcnParticle_t* bp;
    float minWeight, dx, dy, dz, pRange, bcnSum;
    
    minWeight = MIN_WEIGHT(range);
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        tp = &pf->pTag[i];
        bcnSum = 0.0f;
        for (j = 0; j < PF_N_BCN; ++j)
        {
            bp = &bcn->pBcn[i][j];
            dx = tp->x - bp->x;
            dy = tp->y - bp->y;
            dz = tp->z - bp->z;
            pRange = sqrtf(dx * dx + dy * dy + dz * dz);
            if (fabsf(pRange - range) > 3 * stdRange)
                bp->w *= minWeight;
            bcnSum += bp->w;
        }
        tp->w *= bcnSum;
    }
}
