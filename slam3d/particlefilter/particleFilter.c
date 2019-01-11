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
#include <time.h>

#include "particleFilter.h"

#define VIO_STD_XYZ         (1e-3f)
#define VIO_STD_THETA       (1e-6f)

#define MIN_WEIGHT(range)   ((range < 3.0f) ? 0.1f : 0.5f)

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (1.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _initTag(tag_t* tag);
static void _initBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange);
static void _applyVio(tag_t* tag, float dt, float dx, float dy, float dz, float ddist);
static void _applyUwb(tag_t* tag, bcn_t* bcn, float range, float stdRange);
static void _resampleAll(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange);
static void _resampleBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange, uint8_t force);
static void _spawnTagParticle(tagParticle_t* tp);
static void _spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange);

static uint8_t _haveBcn(const particleFilter_t* pf, const bcn_t* bcn);
static int _floatCmp(const void* a, const void* b);
static float _randomUniform(void);
static void _randomNormal2(float* x, float* y);

void particleFilter_init(particleFilter_t* pf)
{
    srand((uint32_t)time(NULL));
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
    _initTag(&pf->tag);
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
    _applyVio(&pf->tag, dt, dx, dy, dz, ddist);
    if (_haveBcn(pf, bcn))
    {
        _applyUwb(&pf->tag, bcn, range, stdRange);
        _resampleAll(&pf->tag, bcn, pf->firstBcn, range, stdRange);
    }
    else
    {
        bcn->nextBcn = pf->firstBcn;
        pf->firstBcn = bcn;
        _initBcn(bcn, &pf->tag, range, stdRange);
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

static void _initTag(tag_t* tag)
{
    int i;
    
    for (i = 0; i < PF_N_TAG; ++i)
        _spawnTagParticle(&tag->pTag[i]);
}

static void _initBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange)
{
    int i, j;
    const tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &tag->pTag[i];
        for (j = 0; j < PF_N_BCN; ++j)
            _spawnBcnParticle(&bcn->pBcn[i][j], tp, range, stdRange);
    }
}

static void _applyVio(tag_t* tag, float dt, float dx, float dy, float dz, float ddist)
{
	int i;
	tagParticle_t* tp;
	float c, s, pDx, pDy, stdXyz, stdTheta;
	float rx, ry, rz, rtheta;

    stdXyz = sqrtf(ddist) * VIO_STD_XYZ;
    stdTheta = sqrtf(dt) * VIO_STD_THETA;
	for (i = 0; i < PF_N_TAG; ++i)
	{
		tp = &tag->pTag[i];
		c = cosf(tp->theta);
		s = sinf(tp->theta);
		pDx = dx * c - dy * s;
		pDy = dx * s + dy * c;

		_randomNormal2(&rx, &ry);
		_randomNormal2(&rz, &rtheta);

		tp->x += pDx + stdXyz * rx;
		tp->y += pDy + stdXyz * ry;
		tp->z += dz + stdXyz * rz;
		tp->theta = fmodf(tp->theta + stdTheta * rtheta, 2 * (float)M_PI);
	}
}

static void _applyUwb(tag_t* tag, bcn_t* bcn, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    bcnParticle_t* bp;
    float minWeight, dx, dy, dz, pRange, bcnSum;
    
    minWeight = MIN_WEIGHT(range);
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &tag->pTag[i];
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
            randCdf[i] = _randomUniform() * s;
        qsort(randCdf, PF_N_TAG, sizeof(float), _floatCmp);
        
        i = 0;
        j = 0;
        while (i < PF_N_TAG)
        {
            while (i < PF_N_TAG && randCdf[i] < weightCdf[j])
            {
                tp = &tag->pTagBuf[i];
                tp2 = &tag->pTag[j];
                
                _randomNormal2(&dx, &dy);
                _randomNormal2(&dz, &dtheta);
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
                randCdf[i] = _randomUniform() * s;
            qsort(randCdf, PF_N_BCN, sizeof(float), _floatCmp);
            
            i = 0;
            j = 0;
            while (i < PF_N_BCN)
            {
                while (i < PF_N_BCN && randCdf[i] < weightCdf[j])
                {
                    bp = &bcn->pBcnBuf[i];
                    bp2 = &bcn->pBcn[k][j];
                    
                    _randomNormal2(&dx, &dy);
                    _randomNormal2(&dz, &dtheta);
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
                _spawnBcnParticle(&bcn->pBcn[k][i], tp, range, stdRange);
        }
        else
        {
            m = PF_N_BCN / s;
            for (i = 0; i < PF_N_BCN; ++i)
                bcn->pBcn[k][i].w *= m;
        }
    }
}

static void _spawnTagParticle(tagParticle_t* tp)
{
    tp->w = 1.0f;
    tp->x = 0.0f;
    tp->y = 0.0f;
    tp->z = 0.0f;
    tp->theta = 0.0f;
}

static void _spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange)
{
    int i;
    float rdist, rdistTmp, relev, razim;
    float c, dx, dy, dz;
    
    rdist = 0.0f;
    for (i = 0; i < 10; ++i)
    {
        rdistTmp = range + 3 * stdRange * (_randomUniform() * 2 - 1);
        if (rdistTmp < 0.0f)
            continue;
        rdist = rdistTmp;
        break;
    }
    
    relev = asinf(_randomUniform() * 2 - 1);
    razim = _randomUniform() * 2 * (float)M_PI;
    
    c = rdist * cosf(relev);
    dx = c * cosf(razim);
    dy = c * sinf(razim);
    dz = rdist * sinf(relev);
    
    bp->w = 1.0f;
    bp->x = tp->x + dx;
    bp->y = tp->y + dy;
    bp->z = tp->z + dz;
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

static float _randomUniform(void)
{
	return (float)rand() / RAND_MAX;
}

static void _randomNormal2(float* x, float* y)
{
	float f = sqrtf(-2 * logf(_randomUniform()));
	float g = _randomUniform() * 2 * (float)M_PI;
    
	*x = f * cosf(g);
	*y = f * sinf(g);
}
