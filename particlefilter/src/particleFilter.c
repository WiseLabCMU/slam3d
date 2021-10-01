/*
 * particleFilter.c
 * Created by John Miller on 11/1/18.
 *
 * Copyright (c) 2018, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <math.h>
#include <stdint.h>

#include "particleFilter.h"
#include "pfInit.h"
#include "pfMeasurement.h"
#include "pfRandom.h"
#include "pfResample.h"

static void _commitVioLoc(particleFilterLoc_t* pf);
static void _commitTagVioSlam(particleFilterSlam_t* pf);
static void _commitBcnVioSlam(bcn_t* bcn);

void particleFilterSeed_set(unsigned int seed)
{
  PF_SEED = seed;
  PF_SEED_SET = 1;
}

void particleFilterLoc_init(particleFilterLoc_t* pf)
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
    pfRandom_init();
    pf->initialized = 0;
}

void particleFilterSlam_init(particleFilterSlam_t* pf)
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
    pfRandom_init();
    pfInit_initTagSlam(pf);
    pf->initialized = 1;
}

void particleFilterSlam_addBcn(bcn_t* bcn)
{
    bcn->initialized = 0;
}

void particleFilterLoc_depositVio(particleFilterLoc_t* pf, double t, float x, float y, float z, float dist)
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

void particleFilterSlam_depositTagVio(particleFilterSlam_t* pf, double t, float x, float y, float z, float dist)
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

void particleFilterSlam_depositBcnVio(bcn_t* bcn, double t, float x, float y, float z, float dist)
{
    float dx, dy, dz;

    if (bcn->firstT == 0.0)
    {
        bcn->firstT = t;
        bcn->firstX = x;
        bcn->firstY = y;
        bcn->firstZ = z;
        bcn->firstDist = dist;
        bcn->lastT = t;
        bcn->lastX = x;
        bcn->lastY = y;
        bcn->lastZ = z;
        bcn->lastDist = dist;
        return;
    }

    if (dist > bcn->lastDist)
    {
        bcn->lastDist = dist;
    }
    else
    {
        dx = x - bcn->lastX;
        dy = y - bcn->lastY;
        dz = z - bcn->lastZ;
        bcn->lastDist += sqrtf(dx * dx + dy * dy + dz * dz);
    }
    bcn->lastT = t;
    bcn->lastX = x;
    bcn->lastY = y;
    bcn->lastZ = z;
}

void particleFilterLoc_depositRange(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
{
    _commitVioLoc(pf);
    if (pf->initialized)
    {
        pfMeasurement_applyRangeLoc(pf, bx, by, bz, range, stdRange);
        pfResample_resampleLoc(pf, bx, by, bz, range, stdRange);
    }
    else
    {
        pfInit_initTagLoc(pf, bx, by, bz, range, stdRange);
        pf->initialized = 1;
    }
}

void particleFilterSlam_depositRange(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns)
{
    int i;

    _commitTagVioSlam(pf);
    for (i = 0; i < numBcns; ++i)
        _commitBcnVioSlam(allBcns[i]);

    if (bcn->initialized)
    {
        pfMeasurement_applyRangeSlam(pf, bcn, range, stdRange);
        pfResample_resampleSlam(pf, bcn, range, stdRange, allBcns, numBcns);
    }
    else
    {
        pfInit_initBcnSlam(bcn, pf, range, stdRange);
        bcn->initialized = 1;
    }
}

void particleFilterLoc_depositRssi(particleFilterLoc_t* pf, float bx, float by, float bz, int rssi)
{
    _commitVioLoc(pf);
    if (pf->initialized)
    {
        pfMeasurement_applyRangeLoc(pf, bx, by, bz, 1.5f, 0.5f);
        pfResample_resampleLoc(pf, bx, by, bz, 1.5f, 0.5f);
    }
    else
    {
        pfInit_initTagLoc(pf, bx, by, bz, 1.5f, 0.5f);
        pf->initialized = 1;
    }
}

void particleFilterSlam_depositRssi(particleFilterSlam_t* pf, bcn_t* bcn, int rssi, bcn_t** allBcns, int numBcns)
{
    int i;

    _commitTagVioSlam(pf);
    for (i = 0; i < numBcns; ++i)
        _commitBcnVioSlam(allBcns[i]);

    if (bcn->initialized)
    {
        pfMeasurement_applyRangeSlam(pf, bcn, 1.5f, 0.5f);
        pfResample_resampleSlam(pf, bcn, 1.5f, 0.5f, allBcns, numBcns);
    }
    else
    {
        pfInit_initBcnSlam(bcn, pf, 1.5f, 0.5f);
        bcn->initialized = 1;
    }
}

uint8_t particleFilterLoc_getTagLoc(const particleFilterLoc_t* pf, double* t, float* x, float* y, float* z, float* theta)
{
    int i;
    const tagParticle_t* tp;
    float w, s, xsum, ysum, zsum, csum, ssum, dx, dy, dz, co, si;
    
    if (!pf->initialized)
        return 0;

    s = 0.0f;
    xsum = 0.0f;
    ysum = 0.0f;
    zsum = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG_LOC; ++i)
    {
        tp = &pf->pTag[i];
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

uint8_t particleFilterSlam_getTagLoc(const particleFilterSlam_t* pf, double* t, float* x, float* y, float* z, float* theta)
{
    int i;
    const tagParticle_t* tp;
    float w, s, xsum, ysum, zsum, csum, ssum, dx, dy, dz, co, si;

    if (!pf->initialized)
        return 0;

    s = 0.0f;
    xsum = 0.0f;
    ysum = 0.0f;
    zsum = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        tp = &pf->pTag[i];
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

uint8_t particleFilterSlam_getBcnLoc(const particleFilterSlam_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z, float* theta)
{
    int i, j;
    const bcnParticle_t* bp;
    float w1, w2, s1, s2, xsum1, xsum2, ysum1, ysum2, zsum1, zsum2, csum1, csum2, ssum1, ssum2;
    
    if (!bcn->initialized)
        return 0;
    
    s1 = 0.0f;
    xsum1 = 0.0f;
    ysum1 = 0.0f;
    zsum1 = 0.0f;
    csum1 = 0.0f;
    ssum1 = 0.0f;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        w1 = pf->pTag[i].w;
        s1 += w1;
        s2 = 0.0f;
        xsum2 = 0.0f;
        ysum2 = 0.0f;
        zsum2 = 0.0f;
        csum2 = 0.0f;
        ssum2 = 0.0f;
        for (j = 0; j < PF_N_BCN; ++j)
        {
            bp = &bcn->pBcn[i][j];
            w2 = bp->w;
            s2 += w2;
            xsum2 += w2 * bp->x;
            ysum2 += w2 * bp->y;
            zsum2 += w2 * bp->z;
            csum2 += w2 * cosf(bp->theta);
            ssum2 += w2 * sinf(bp->theta);
        }
        xsum1 += w1 * xsum2 / s2;
        ysum1 += w1 * ysum2 / s2;
        zsum1 += w1 * zsum2 / s2;
        csum1 += w1 * csum2 / s2;
        ssum1 += w1 * ssum2 / s2;
    }
    *t = pf->lastT;
    *x = xsum1 / s1;
    *y = ysum1 / s1;
    *z = zsum1 / s1;
    *theta = atan2f(ssum1, csum1);

    return 1;
}

static void _commitVioLoc(particleFilterLoc_t* pf)
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
    pfMeasurement_applyVioLoc(pf, dt, dx, dy, dz, ddist);
}

static void _commitTagVioSlam(particleFilterSlam_t* pf)
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
    pfMeasurement_applyTagVioSlam(pf, dt, dx, dy, dz, ddist);
}

static void _commitBcnVioSlam(bcn_t* bcn)
{
    float dt, dx, dy, dz, ddist;

    dt = (float)(bcn->lastT - bcn->firstT);
    dy = bcn->lastY - bcn->firstY;
    dz = bcn->lastZ - bcn->firstZ;
    dx = bcn->lastX - bcn->firstX;
    ddist = bcn->lastDist - bcn->firstDist;
    bcn->firstT = bcn->lastT;
    bcn->firstX = bcn->lastX;
    bcn->firstY = bcn->lastY;
    bcn->firstZ = bcn->lastZ;
    bcn->firstDist = bcn->lastDist;
    pfMeasurement_applyBcnVioSlam(bcn, dt, dx, dy, dz, ddist);
}
