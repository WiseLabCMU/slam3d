//
//  particleFilter.c
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#include <math.h>
#include <stdint.h>

#include "particleFilter.h"
#include "pfInit.h"
#include "pfMeasurement.h"
#include "pfResample.h"

static void _commitVio(particleFilter_t* pf);

void particleFilter_init(particleFilter_t* pf)
{
    pf->firstT = 0.0;
    pf->firstX = 0.0f;
    pf->firstY = 0.0f;
    pf->firstZ = 0.0f;
    pf->firstDist = 0.0f;
    pf->lastT = 0.0;
    pf->lastX = 0.0f;
    pf->lastY = 0.0f;
    pf->lastZ = 0.0f;
    pf->lastDist = 0.0f;
    pf->tag.initialized = 0;
}

void particleFilter_initSlam(particleFilter_t* pf)
{
    pf->firstT = 0.0;
    pf->firstX = 0.0f;
    pf->firstY = 0.0f;
    pf->firstZ = 0.0f;
    pf->firstDist = 0.0f;
    pf->lastT = 0.0;
    pf->lastX = 0.0f;
    pf->lastY = 0.0f;
    pf->lastZ = 0.0f;
    pf->lastDist = 0.0f;
    pfInit_initTagSlam(&pf->tag);
    pf->tag.initialized = 1;
}

void particleFilter_addBcn(bcn_t* bcn)
{
    bcn->initialized = 0;
}

void particleFilter_depositVio(particleFilter_t* pf, double t, float x, float y, float z, float dist)
{
    float dx, dy, dz;
    
    if (pf->firstT == 0.0)
    {
        pf->firstT = t;
        pf->firstX = x;
        pf->firstY = y;
        pf->firstZ = z;
        pf->firstDist = dist;
        pf->lastT = t;
        pf->lastX = x;
        pf->lastY = y;
        pf->lastZ = z;
        pf->lastDist = dist;
        return;
    }
    
    if (dist > pf->lastDist)
    {
        pf->lastDist = dist;
    }
    else
    {
        dx = x - pf->lastX;
        dy = y - pf->lastY;
        dz = z - pf->lastZ;
        pf->lastDist += sqrtf(dx * dx + dy * dy + dz * dz);
    }
    pf->lastT = t;
    pf->lastX = x;
    pf->lastY = y;
    pf->lastZ = z;
}

void particleFilter_depositRange(particleFilter_t* pf, float bx, float by, float bz, float range, float stdRange)
{
    _commitVio(pf);
    if (pf->tag.initialized)
    {
        pfMeasurement_applyRange(&pf->tag, bx, by, bz, range, stdRange);
        pfResample_resample(&pf->tag, bx, by, bz, range, stdRange);
    }
    else
    {
        pfInit_initTag(&pf->tag, bx, by, bz, range, stdRange);
        pf->tag.initialized = 1;
    }
}

void particleFilter_depositRangeSlam(particleFilter_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns)
{
    _commitVio(pf);
    if (bcn->initialized)
    {
        pfMeasurement_applyRangeSlam(&pf->tag, bcn, range, stdRange);
        pfResample_resampleSlam(&pf->tag, bcn, range, stdRange, allBcns, numBcns);
    }
    else
    {
        pfInit_initBcn(bcn, &pf->tag, range, stdRange);
        bcn->initialized = 1;
    }
}

void particleFilter_depositRssi(particleFilter_t* pf, float bx, float by, float bz, int rssi)
{
    _commitVio(pf);
    if (pf->tag.initialized)
    {
        pfMeasurement_applyRange(&pf->tag, bx, by, bz, 1.5f, 0.5f);
        pfResample_resample(&pf->tag, bx, by, bz, 1.5f, 0.5f);
    }
    else
    {
        pfInit_initTag(&pf->tag, bx, by, bz, 1.5f, 0.5f);
        pf->tag.initialized = 1;
    }
}

void particleFilter_depositRssiSlam(particleFilter_t* pf, bcn_t* bcn, int rssi, bcn_t** allBcns, int numBcns)
{
    _commitVio(pf);
    if (bcn->initialized)
    {
        pfMeasurement_applyRangeSlam(&pf->tag, bcn, 1.5f, 0.5f);
        pfResample_resampleSlam(&pf->tag, bcn, 1.5f, 0.5f, allBcns, numBcns);
    }
    else
    {
        pfInit_initBcn(bcn, &pf->tag, 1.5f, 0.5f);
        bcn->initialized = 1;
    }
}

uint8_t particleFilter_getTagLoc(const particleFilter_t* pf, double* t, float* x, float* y, float* z, float* theta)
{
    int i;
    const tagParticle_t* tp;
    float w, s, xsum, ysum, zsum, csum, ssum, dx, dy, dz, co, si;
    
    if (!pf->tag.initialized)
        return 0;

    s = 0.0f;
    xsum = 0.0f;
    ysum = 0.0f;
    zsum = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->tag.pTag[i];
        w = tp->w;
        s += w;
        xsum += w * tp->x;
        ysum += w * tp->y;
        zsum += w * tp->z;
        csum += w * cosf(tp->theta);
        ssum += w * sinf(tp->theta);
    }
    *t = pf->lastT;
    *x = xsum / s;
    *y = ysum / s;
    *z = zsum / s;
    *theta = atan2f(ssum, csum);
    
    dx = pf->lastX - pf->firstX;
    dy = pf->lastY - pf->firstY;
    dz = pf->lastZ - pf->firstZ;
    
    co = cosf(*theta);
    si = sinf(*theta);
    *x += dx * co - dy * si;
    *y += dx * si + dy * co;
    *z += dz;

    return 1;
}

uint8_t particleFilter_getBcnLoc(const particleFilter_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z)
{
    int i, j;
    const bcnParticle_t* bp;
    float w1, w2, s1, s2, xsum1, xsum2, ysum1, ysum2, zsum1, zsum2;
    
    if (!bcn->initialized)
        return 0;
    
    s1 = 0.0f;
    xsum1 = 0.0f;
    ysum1 = 0.0f;
    zsum1 = 0.0f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        w1 = pf->tag.pTag[i].w;
        s1 += w1;
        s2 = 0.0f;
        xsum2 = 0.0f;
        ysum2 = 0.0f;
        zsum2 = 0.0f;
        for (j = 0; j < PF_N_BCN; ++j)
        {
            bp = &bcn->pBcn[i][j];
            w2 = bp->w;
            s2 += w2;
            xsum2 += w2 * bp->x;
            ysum2 += w2 * bp->y;
            zsum2 += w2 * bp->z;
        }
        xsum1 += w1 * xsum2 / s2;
        ysum1 += w1 * ysum2 / s2;
        zsum1 += w1 * zsum2 / s2;
    }
    *t = pf->lastT;
    *x = xsum1 / s1;
    *y = ysum1 / s1;
    *z = zsum1 / s1;

    return 1;
}

static void _commitVio(particleFilter_t* pf)
{
    float dt, dx, dy, dz, ddist;

    dt = (float)(pf->lastT - pf->firstT);
    dx = pf->lastX - pf->firstX;
    dy = pf->lastY - pf->firstY;
    dz = pf->lastZ - pf->firstZ;
    ddist = pf->lastDist - pf->firstDist;
    pf->firstT = pf->lastT;
    pf->firstX = pf->lastX;
    pf->firstY = pf->lastY;
    pf->firstZ = pf->lastZ;
    pf->firstDist = pf->lastDist;
    pfMeasurement_applyVio(&pf->tag, dt, dx, dy, dz, ddist);
}
