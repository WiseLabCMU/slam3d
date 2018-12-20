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
#include <time.h>

#include "particleFilter.h"

#define VIO_STD_XYZ         (1e-3f)
#define VIO_STD_THETA       (1e-6f)

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (1.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _initTag(particleFilter_t* pf);
static void _initBcn(particleFilter_t* pf, bcn_t* b, float range, float stdRange);
static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist);
static void _applyUwb(particleFilter_t* pf, bcn_t* b, float range, float stdRange);
static void _resampleAll(particleFilter_t* pf, bcn_t* b, float range, float stdRange);
static void _resampleBcn(bcn_t* b, const particleFilter_t* pf, float range, float stdRange, uint8_t force);
static void _spawnTagParticle(tagParticle_t* tp);
static void _spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange);

static bcn_t* _getBcn(const particleFilter_t* pf, uint32_t bcnId);
static int _floatCmp(const void* a, const void* b);
static float _randomUniform(void);
static void _randomNormal2(float* x, float* y);

void particleFilter_init(particleFilter_t* pf)
{
    srand((uint32_t)time(NULL));
    pf->firstBcn = NULL;
    pf->pTag = pf->pTagBuf1;
    pf->pTagTmp = pf->pTagBuf2;
    pf->totalDt = 0.0f;
    pf->totalDx = 0.0f;
    pf->totalDy = 0.0f;
    pf->totalDz = 0.0f;
    pf->totalDist = 0.0f;
    _initTag(pf);
}

void particleFilter_addBcn(particleFilter_t* pf, bcn_t* b, uint32_t bcnId, float range, float stdRange)
{
    b->pBcn = b->pBcnBuf1;
    b->pBcnTmp = b->pBcnBuf2;
    b->bcnId = bcnId;
    b->nextBcn = pf->firstBcn;
    pf->firstBcn = b;
    _initBcn(pf, b, range, stdRange);
}

void particleFilter_depositVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist)
{    
    pf->totalDt += dt;
    pf->totalDx += dx;
    pf->totalDy += dy;
    pf->totalDz += dz;
    pf->totalDist += dist;
}

void particleFilter_depositUwb(particleFilter_t* pf, uint32_t bcnId, float range, float stdRange)
{
    bcn_t* b;
    
    b = _getBcn(pf, bcnId);
    _applyVio(pf, pf->totalDt, pf->totalDx, pf->totalDy, pf->totalDz, pf->totalDist);
    _applyUwb(pf, b, range, stdRange);
    _resampleAll(pf, b, range, stdRange);
    pf->totalDt = 0.0f;
    pf->totalDx = 0.0f;
    pf->totalDy = 0.0f;
    pf->totalDz = 0.0f;
    pf->totalDist = 0.0f;
}

static void _initTag(particleFilter_t* pf)
{
    int i;
    
    for (i = 0; i < PF_N_TAG; ++i)
        _spawnTagParticle(&pf->pTag[i]);
}

static void _initBcn(particleFilter_t* pf, bcn_t* b, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        for (j = 0; j < PF_N_BCN; ++j)
            _spawnBcnParticle(&b->pBcn[i][j], tp, range, stdRange);
    }
}

static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist)
{
	int i;
	tagParticle_t* tp;
	float c, s, pDx, pDy, stdXyz, stdTheta;
	float rx, ry, rz, rtheta;

    stdXyz = sqrtf(dist) * VIO_STD_XYZ;
    stdTheta = sqrtf(dt) * VIO_STD_THETA;
	for (i = 0; i < PF_N_TAG; ++i)
	{
		tp = &pf->pTag[i];
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

static void _applyUwb(particleFilter_t* pf, bcn_t* b, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    bcnParticle_t* bp;
    float minWeight, dx, dy, dz, pRange, bcnSum;
    
    minWeight = (range < 3.0f) ? 0.1f : 0.5f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        bcnSum = 0.0f;
        for (j = 0; j < PF_N_BCN; ++j)
        {
            bp = &b->pBcn[i][j];
            dx = tp->x - bp->x;
            dy = tp->y - bp->y;
            dz = tp->z - bp->z;
            pRange = sqrtf(dx * dx + dy * dy + dz * dz);
            if (fabsf(pRange - range) > 3 * stdRange)
            {
                bp->w *= minWeight;
                bcnSum += bp->w;
            }
        }
        tp->w *= bcnSum;
    }
}

static void _resampleAll(particleFilter_t* pf, bcn_t* b, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    tagParticle_t* tp2;
    bcn_t* bcn;
    float w, s, ss, csum, ssum, ess, htheta, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_TAG];
    float randCdf[PF_N_TAG];

    s = 0.0f;
    ss = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
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
                tp = &pf->pTagTmp[i];
                tp2 = &pf->pTag[j];
                
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

        tp = pf->pTag;
        pf->pTag = pf->pTagTmp;
        pf->pTagTmp = tp;

        for (bcn = pf->firstBcn; bcn != NULL; bcn = bcn->nextBcn)
            _resampleBcn(bcn, pf, range, stdRange, 1);
    }
    else
    {
        m = PF_N_TAG / s;
        for (i = 0; i < PF_N_TAG; ++i)
            pf->pTag[i].w *= m;
        _resampleBcn(b, pf, range, stdRange, 0);
    }
}

static void _resampleBcn(bcn_t* b, const particleFilter_t* pf, float range, float stdRange, uint8_t force)
{
    int numSpawn, i, j, k;
    tagParticle_t* tp;
    bcnParticle_t* bp;
    bcnParticle_t* bp2;
    bcnParticle_t (* bpt)[PF_N_BCN];
    float w, s, ss, ess, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_BCN];
    float randCdf[PF_N_BCN];
    
    for (k = 0; k < PF_N_TAG; ++k)
    {
        s = 0.0f;
        ss = 0.0f;
        for (i = 0; i < PF_N_BCN; ++i)
        {
            bp = &b->pBcn[k][i];
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
            for (i = 0; i < PF_N_TAG; ++i)
                randCdf[i] = _randomUniform() * s;
            qsort(randCdf, PF_N_TAG, sizeof(float), _floatCmp);
            
            i = 0;
            j = 0;
            while (i < PF_N_BCN)
            {
                while (i < PF_N_BCN && randCdf[i] < weightCdf[j])
                {
                    bp = &b->pBcnTmp[k][i];
                    bp2 = &b->pBcn[k][j];
                    
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
            
            bpt = b->pBcn;
            b->pBcn = b->pBcnTmp;
            b->pBcnTmp = bpt;
            
            tp = &pf->pTag[k];
            for (i = 0; i < numSpawn; ++i)
                _spawnBcnParticle(&b->pBcn[k][i], tp, range, stdRange);
        }
        
        m = PF_N_BCN / s;
        for (i = 0; i < PF_N_BCN; ++i)
            b->pBcn[k][i].w *= m;
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
    float rdist, relev, razim;
    float c, dx, dy, dz;
    
    do
        rdist = range + 3 * stdRange * (_randomUniform() * 2 - 1);
    while (rdist < 0);
    
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

static bcn_t* _getBcn(const particleFilter_t* pf, uint32_t bcnId)
{
    bcn_t* b;
    for (b = pf->firstBcn; b != NULL; b = b->nextBcn)
        if (b->bcnId == bcnId)
            return b;
    return NULL;
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
