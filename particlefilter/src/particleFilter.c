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

static void _commitVioLoc(particleFilterLoc_t* pf);
static void _commitVioSlam(particleFilterSlam_t* pf);

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

void particleFilterSlam_depositVio(particleFilterSlam_t* pf, double t, float x, float y, float z, float dist)
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
    _commitVioSlam(pf);
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
    _commitVioSlam(pf);
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

uint8_t particleFilterSlam_getBcnLoc(const particleFilterSlam_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z)
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
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
    {
        w1 = pf->pTag[i].w;
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

static void _commitVioSlam(particleFilterSlam_t* pf)
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
    pfMeasurement_applyVioSlam(pf, dt, dx, dy, dz, ddist);
}
