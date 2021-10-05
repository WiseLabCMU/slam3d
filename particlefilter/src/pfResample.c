/*
 * pfResample.c
 * Created by John Miller on 11/1/19.
 *
 * Copyright (c) 2019, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pfInit.h"
#include "pfRandom.h"
#include "pfResample.h"

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (4.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _resampleBcn(bcn_t* bcn, const particleFilterSlam_t* pf, float range, float stdRange, uint8_t force);

void pfResample_resampleLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
{
    int numSpawn, i, j;
    tagParticle_t* tp;
    float invN, w, s, ss, csum, ssum, ess, htheta, m, rStart, rStep;
    float weightCdf[PF_N_TAG_LOC];

    s = 0.0f;
    ss = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG_LOC; ++i)
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

    invN = 1.0f / PF_N_TAG_LOC;
    numSpawn = 0;
    if (s * invN < WEIGHT_SPAWN_THRESH && range < RADIUS_SPAWN_THRESH)
        numSpawn = (int)lroundf(PF_N_BCN * PCT_SPAWN);

    if (ess * invN < RESAMPLE_THRESH || numSpawn > 0)
    {
        csum /= s;
        ssum /= s;
        htheta = csum * csum + ssum * ssum;
        htheta = htheta > 1e-10f ? htheta : 1e-10f;
        htheta = htheta < 1 - 1e-10f ? htheta : 1 - 1e-10f;
        htheta = sqrtf(-logf(htheta) / ess);

        rStep = invN * s;
        rStart = pfRandom_uniform() * rStep;

        for (i = 0, j = 0; i < PF_N_TAG_LOC; ++j)
            for (; i < PF_N_TAG_LOC && (rStart + rStep * i) < weightCdf[j]; ++i)
                pfInit_spawnTagParticleFromOther(&pf->pTagBuf[i], &pf->pTag[j], HXYZ, htheta);

        memcpy(pf->pTag, pf->pTagBuf, sizeof(pf->pTagBuf));
        for (i = 0; i < numSpawn; ++i)
            pfInit_spawnTagParticleFromRange(&pf->pTag[i], bx, by, bz, range, stdRange);
    }
    else
    {
        m = PF_N_TAG_LOC / s;
        for (i = 0; i < PF_N_TAG_LOC; ++i)
            pf->pTag[i].w *= m;
    }
}

void pfResample_resampleSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns)
{
    int i, j;
    tagParticle_t* tp;
    float invN, w, s, ss, csum, ssum, ess, htheta, m, rStart, rStep;
    float weightCdf[PF_N_TAG_SLAM];
    
    s = 0.0f;
    ss = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
    for (i = 0; i < PF_N_TAG_SLAM; ++i)
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
    
    invN = 1.0f / PF_N_TAG_SLAM;
    if (ess * invN < RESAMPLE_THRESH)
    {
        csum /= s;
        ssum /= s;
        htheta = csum * csum + ssum * ssum;
        htheta = htheta > 1e-10f ? htheta : 1e-10f;
        htheta = htheta < 1 - 1e-10f ? htheta : 1 - 1e-10f;
        htheta = sqrtf(-logf(htheta) / ess);
        
        rStep = invN * s;
        rStart = pfRandom_uniform() * rStep;
        
        for (i = 0, j = 0; i < PF_N_TAG_SLAM; ++j)
            for (; i < PF_N_TAG_SLAM && (rStart + rStep * i) < weightCdf[j]; ++i)
                pfInit_spawnTagParticleFromOther(&pf->pTagBuf[i], &pf->pTag[j], HXYZ, htheta);
        
        memcpy(pf->pTag, pf->pTagBuf, sizeof(pf->pTagBuf));
        
        for (i = 0; i < numBcns; ++i)
            if (allBcns[i]->initialized)
                _resampleBcn(allBcns[i], pf, range, stdRange, 1);
    }
    else
    {
        m = PF_N_TAG_SLAM / s;
        for (i = 0; i < PF_N_TAG_SLAM; ++i)
            pf->pTag[i].w *= m;
        _resampleBcn(bcn, pf, range, stdRange, 0);
    }
}

static void _resampleBcn(bcn_t* bcn, const particleFilterSlam_t* pf, float range, float stdRange, uint8_t force)
{
    int numSpawn, i, j, k;
    const tagParticle_t* tp;
    bcnParticle_t* bp;
    float invN, w, s, ss, csum, ssum, ess, htheta, m, rStart, rStep;
    float weightCdf[PF_N_BCN];
    
    for (k = 0; k < PF_N_TAG_SLAM; ++k)
    {
        s = 0.0f;
        ss = 0.0f;
        csum = 0.0f;
        ssum = 0.0f;
        for (i = 0; i < PF_N_BCN; ++i)
        {
            bp = &bcn->pBcn[k][i];
            w = bp->w;
            s += w;
            ss += w * w;
            csum += w * cosf(bp->theta);
            ssum += w * sinf(bp->theta);
            weightCdf[i] = s;
        }
        ess = s * s / ss;
        
        invN = 1.0f / PF_N_BCN;
        numSpawn = 0;
        if (s * invN < WEIGHT_SPAWN_THRESH && range < RADIUS_SPAWN_THRESH)
            numSpawn = (int)lroundf(PF_N_BCN * PCT_SPAWN);
        
        if (ess * invN < RESAMPLE_THRESH || numSpawn > 0 || force)
        {
            csum /= s;
            ssum /= s;
            htheta = csum * csum + ssum * ssum;
            htheta = htheta > 1e-10f ? htheta : 1e-10f;
            htheta = htheta < 1 - 1e-10f ? htheta : 1 - 1e-10f;
            htheta = sqrtf(-logf(htheta) / ess);

            rStep = invN * s;
            rStart = pfRandom_uniform() * rStep;
            
            for (i = 0, j = 0; i < PF_N_BCN; ++j)
                for (; i < PF_N_BCN && (rStart + rStep * i) < weightCdf[j]; ++i)
                    pfInit_spawnBcnParticleFromOther(&bcn->pBcnBuf[i], &bcn->pBcn[k][j], HXYZ, htheta);
            
            memcpy(bcn->pBcn[k], bcn->pBcnBuf, sizeof(bcn->pBcnBuf));
            
            tp = &pf->pTag[k];
            for (i = 0; i < numSpawn; ++i)
                pfInit_spawnBcnParticleFromRange(&bcn->pBcn[k][i], tp, range, stdRange);
        }
        else
        {
            m = PF_N_BCN / s;
            for (i = 0; i < PF_N_BCN; ++i)
                bcn->pBcn[k][i].w *= m;
        }
    }
}
