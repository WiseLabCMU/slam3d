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
static void _initBeacon(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _spawnBeaconParticle(beaconParticle_t* bp, const tagParticle_t* tp, float range, float std_range);
static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist);
static void _applyUwb(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _resampleBeacon(const particleFilter_t* pf, beacon_t* b, float range, float std_range, uint8_t force);

static beacon_t* _getBeacon(const particleFilter_t* pf, uint32_t beaconId);
static int _floatCmp(const void* a, const void* b);
static float _randomUniform(void);
static void _randomNormal2(float* x, float* y);

void particleFilter_init(particleFilter_t* pf)
{
    srand((uint32_t)time(NULL));
    pf->firstBeacon = NULL;
    pf->pTag = pf->pTagBuf1;
    pf->pTagTmp = pf->pTagBuf2;
    pf->totalDt = 0.0f;
    pf->totalDx = 0.0f;
    pf->totalDy = 0.0f;
    pf->totalDz = 0.0f;
    pf->totalDist = 0.0f;
    _initTag(pf);
}

void particleFilter_addBeacon(particleFilter_t* pf, beacon_t* b, uint32_t beaconId, float range, float std_range)
{
    b->pBeacon = b->pBeaconBuf1;
    b->pBeaconTmp = b->pBeaconBuf2;
    b->beaconId = beaconId;
    b->nextBeacon = pf->firstBeacon;
    pf->firstBeacon = b;
    _initBeacon(pf, b, range, std_range);
}

void particleFilter_depositVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist)
{    
    pf->totalDt += dt;
    pf->totalDx += dx;
    pf->totalDy += dy;
    pf->totalDz += dz;
    pf->totalDist += dist;
}

void particleFilter_depositUwb(particleFilter_t* pf, uint32_t beaconId, float range, float std_range)
{
    beacon_t* b;
    
    b = _getBeacon(pf, beaconId);
    _applyVio(pf, pf->totalDt, pf->totalDx, pf->totalDy, pf->totalDz, pf->totalDist);
    _applyUwb(pf, b, range, std_range);
    _resample(pf, b, range, std_range);
    pf->totalDt = 0.0f;
    pf->totalDx = 0.0f;
    pf->totalDy = 0.0f;
    pf->totalDz = 0.0f;
    pf->totalDist = 0.0f;
}

static void _initTag(particleFilter_t* pf)
{
    int i;
    tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        tp->w = 1.0f;
        tp->x = 0.0f;
        tp->y = 0.0f;
        tp->z = 0.0f;
        tp->theta = 0.0f;
    }
}

static void _initBeacon(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        for (j = 0; j < PF_N_BEACON; ++j)
            _spawnBeaconParticle(&b->pBeacon[i][j], tp, range, std_range);
    }
}

static void _spawnBeaconParticle(beaconParticle_t* bp, const tagParticle_t* tp, float range, float std_range)
{
    float rdist, relev, razim;
    float c, dx, dy, dz;
    
    do
        rdist = range + 3 * std_range * (_randomUniform() * 2 - 1);
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

static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist)
{
	int i;
	tagParticle_t* tp;
	float c, s, p_dx, p_dy, std_xyz, std_theta;
	float rx, ry, rz, rtheta;

    std_xyz = sqrtf(dist) * VIO_STD_XYZ;
    std_theta = sqrtf(dt) * VIO_STD_THETA;
	for (i = 0; i < PF_N_TAG; ++i)
	{
		tp = &pf->pTag[i];
		c = cosf(tp->theta);
		s = sinf(tp->theta);
		p_dx = dx * c - dy * s;
		p_dy = dx * s + dy * c;

		_randomNormal2(&rx, &ry);
		_randomNormal2(&rz, &rtheta);

		tp->x += p_dx + std_xyz * rx;
		tp->y += p_dy + std_xyz * ry;
		tp->z += dz + std_xyz * rz;
		tp->theta = fmodf(tp->theta + std_theta * rtheta, 2 * (float)M_PI);
	}
}

static void _applyUwb(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    beaconParticle_t* bp;
    float min_weight, dx, dy, dz, p_range, beacon_sum;
    
    min_weight = (range < 3.0f) ? 0.1f : 0.5f;
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        beacon_sum = 0.0f;
        for (j = 0; j < PF_N_BEACON; ++j)
        {
            bp = &b->pBeacon[i][j];
            dx = tp->x - bp->x;
            dy = tp->y - bp->y;
            dz = tp->z - bp->z;
            p_range = sqrtf(dx * dx + dy * dy + dz * dz);
            if (fabsf(p_range - range) > 3 * std_range)
            {
                bp->w *= min_weight;
                beacon_sum += bp->w;
            }
        }
        tp->w *= beacon_sum;
    }
}

static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    tagParticle_t* tp2;
    beacon_t* bcn;
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

        for (bcn = pf->firstBeacon; bcn != NULL; bcn = bcn->nextBeacon)
            _resampleBeacon(pf, bcn, range, std_range, 1);
    }
    else
    {
        m = PF_N_TAG / s;
        for (i = 0; i < PF_N_TAG; ++i)
            pf->pTag[i].w *= m;
        _resampleBeacon(pf, b, range, std_range, 0);
    }
}

static void _resampleBeacon(const particleFilter_t* pf, beacon_t* b, float range, float std_range, uint8_t force)
{
    int numSpawn, i, j, k;
    tagParticle_t* tp;
    beaconParticle_t* bp;
    beaconParticle_t* bp2;
    beaconParticle_t (* bpt)[PF_N_BEACON];
    float w, s, ss, ess, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_BEACON];
    float randCdf[PF_N_BEACON];
    
    for (k = 0; k < PF_N_TAG; ++k)
    {
        s = 0.0f;
        ss = 0.0f;
        for (i = 0; i < PF_N_BEACON; ++i)
        {
            bp = &b->pBeacon[k][i];
            w = bp->w;
            s += w;
            ss += w * w;
            weightCdf[i] = s;
        }
        ess = s * s / ss;
        
        numSpawn = 0;
        if (s / PF_N_BEACON < WEIGHT_SPAWN_THRESH && range < RADIUS_SPAWN_THRESH)
            numSpawn = (int)lroundf(PF_N_BEACON * PCT_SPAWN);
        
        if (ess / PF_N_BEACON < RESAMPLE_THRESH || numSpawn > 0 || force)
        {
            for (i = 0; i < PF_N_TAG; ++i)
                randCdf[i] = _randomUniform() * s;
            qsort(randCdf, PF_N_TAG, sizeof(float), _floatCmp);
            
            i = 0;
            j = 0;
            while (i < PF_N_BEACON)
            {
                while (i < PF_N_BEACON && randCdf[i] < weightCdf[j])
                {
                    bp = &b->pBeaconTmp[k][i];
                    bp2 = &b->pBeacon[k][j];
                    
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
            
            bpt = b->pBeacon;
            b->pBeacon = b->pBeaconTmp;
            b->pBeaconTmp = bpt;
            
            tp = &pf->pTag[k];
            for (i = 0; i < numSpawn; ++i)
                _spawnBeaconParticle(&b->pBeacon[k][i], tp, range, std_range);
        }
        
        m = PF_N_BEACON / s;
        for (i = 0; i < PF_N_BEACON; ++i)
            b->pBeacon[k][i].w *= m;
    }
}

static beacon_t* _getBeacon(const particleFilter_t* pf, uint32_t beaconId)
{
    beacon_t* b;
    for (b = pf->firstBeacon; b != NULL; b = b->nextBeacon)
        if (b->beaconId == beaconId)
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
