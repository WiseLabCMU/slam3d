//
//  particleFilter.c
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "particleFilter.h"
#include "pfInit.h"
#include "pfMeasurement.h"
#include "pfRandom.h"

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (1.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _resampleAll(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange);
static void _resampleBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange, uint8_t force);

static uint8_t _haveBcn(const particleFilter_t* pf, const bcn_t* bcn);
static int _floatCmp(const void* a, const void* b);

void particleFilter_init(particleFilter_t* pf)
{
    pfRandom_init();
    pf->firstBcn = NULL;
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
    pfInit_initTag(&pf->tag);
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

void particleFilter_depositUwb(particleFilter_t* pf, bcn_t* bcn, float range, float stdRange)
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
    if (_haveBcn(pf, bcn))
    {
        pfMeasurement_applyUwb(&pf->tag, bcn, range, stdRange);
        _resampleAll(&pf->tag, bcn, pf->firstBcn, range, stdRange);
    }
    else
    {
        bcn->nextBcn = pf->firstBcn;
        pf->firstBcn = bcn;
        pfInit_initBcn(bcn, &pf->tag, range, stdRange);
    }
}

void particleFilter_getTagLoc(const particleFilter_t* pf, double* t, float* x, float* y, float* z, float* theta)
{
    int i;
    const tagParticle_t* tp;
    float w, s, xsum, ysum, zsum, csum, ssum, dx, dy, dz, co, si;
    
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
}

void particleFilter_getBcnLoc(const particleFilter_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z)
{
    int i, j;
    const bcnParticle_t* bp;
    float w1, w2, s1, s2, xsum1, xsum2, ysum1, ysum2, zsum1, zsum2;
    
    if (!_haveBcn(pf, bcn))
        return;
    
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
}

static void _resampleAll(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    tagParticle_t* tp2;
    float w, s, ss, csum, ssum, ess, htheta, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_TAG];
    float randCdf[PF_N_TAG];

    s = 0.0f;
    ss = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &tag->pTag[i];
        w = tp->w;
        s += w;
        ss += w * w;
        csum += w * cosf(tp->theta);
        ssum += w * sinf(tp->theta);
        weightCdf[i] = s;
    }
    ess = s * s / ss;
    
    if (ess / PF_N_TAG < RESAMPLE_THRESH)
    {
        csum /= s;
        ssum /= s;
        htheta = csum * csum + ssum * ssum;
        htheta = htheta > 1e-10 ? htheta : 1e-10;
        htheta = htheta < 1 - 1e-10 ? htheta : 1 - 1e-10;
        htheta = sqrtf(-logf(htheta) / ess);

        for (i = 0; i < PF_N_TAG; ++i)
            randCdf[i] = pfRandom_uniform() * s;
        qsort(randCdf, PF_N_TAG, sizeof(float), _floatCmp);
        
        i = 0;
        j = 0;
        while (i < PF_N_TAG)
        {
            while (i < PF_N_TAG && randCdf[i] < weightCdf[j])
            {
                tp = &tag->pTagBuf[i];
                tp2 = &tag->pTag[j];
                
                pfRandom_normal2(&dx, &dy);
                pfRandom_normal2(&dz, &dtheta);
                tp->w = 1.0f;
                tp->x = tp2->x + dx * HXYZ;
                tp->y = tp2->y + dy * HXYZ;
                tp->z = tp2->z + dz * HXYZ;
                tp->theta = fmodf(tp2->theta + dtheta * htheta, 2 * (float)M_PI);
                
                ++i;
            }
            ++j;
        }

        memcpy(tag->pTag, tag->pTagBuf, sizeof(tag->pTagBuf));

        for (bcn = firstBcn; bcn != NULL; bcn = bcn->nextBcn)
            _resampleBcn(bcn, tag, range, stdRange, 1);
    }
    else
    {
        m = PF_N_TAG / s;
        for (i = 0; i < PF_N_TAG; ++i)
            tag->pTag[i].w *= m;
        _resampleBcn(bcn, tag, range, stdRange, 0);
    }
}

static void _resampleBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange, uint8_t force)
{
    int numSpawn, i, j, k;
    const tagParticle_t* tp;
    bcnParticle_t* bp;
    bcnParticle_t* bp2;
    float w, s, ss, ess, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_BCN];
    float randCdf[PF_N_BCN];
    
    for (k = 0; k < PF_N_TAG; ++k)
    {
        s = 0.0f;
        ss = 0.0f;
        for (i = 0; i < PF_N_BCN; ++i)
        {
            bp = &bcn->pBcn[k][i];
            w = bp->w;
            s += w;
            ss += w * w;
            weightCdf[i] = s;
        }
        ess = s * s / ss;
        
        numSpawn = 0;
        if (s / PF_N_BCN < WEIGHT_SPAWN_THRESH && range < RADIUS_SPAWN_THRESH)
            numSpawn = (int)lroundf(PF_N_BCN * PCT_SPAWN);
        
        if (ess / PF_N_BCN < RESAMPLE_THRESH || numSpawn > 0 || force)
        {
            for (i = 0; i < PF_N_BCN; ++i)
                randCdf[i] = pfRandom_uniform() * s;
            qsort(randCdf, PF_N_BCN, sizeof(float), _floatCmp);
            
            i = 0;
            j = 0;
            while (i < PF_N_BCN)
            {
                while (i < PF_N_BCN && randCdf[i] < weightCdf[j])
                {
                    bp = &bcn->pBcnBuf[i];
                    bp2 = &bcn->pBcn[k][j];
                    
                    pfRandom_normal2(&dx, &dy);
                    pfRandom_normal2(&dz, &dtheta);
                    bp->w = 1.0f;
                    bp->x = bp2->x + dx * HXYZ;
                    bp->y = bp2->y + dy * HXYZ;
                    bp->z = bp2->z + dz * HXYZ;
                    
                    ++i;
                }
                ++j;
            }
            
            memcpy(bcn->pBcn[k], bcn->pBcnBuf, sizeof(bcn->pBcnBuf));
            
            tp = &tag->pTag[k];
            for (i = 0; i < numSpawn; ++i)
                pfInit_spawnBcnParticle(&bcn->pBcn[k][i], tp, range, stdRange);
        }
        else
        {
            m = PF_N_BCN / s;
            for (i = 0; i < PF_N_BCN; ++i)
                bcn->pBcn[k][i].w *= m;
        }
    }
}

static uint8_t _haveBcn(const particleFilter_t* pf, const bcn_t* bcn)
{
    bcn_t* bcn2;
    
    for (bcn2 = pf->firstBcn; bcn2 != NULL; bcn2 = bcn2->nextBcn)
        if (bcn2 == bcn)
            return 1;
    return 0;
}

static int _floatCmp(const void* a, const void* b)
{
    float* x = (float*)a;
    float* y = (float*)b;
    
    if (*x < *y)
        return -1;
    if (*x > *y)
        return 1;
    return 0;
}
