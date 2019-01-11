//
//  pfResample.c
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pfInit.h"
#include "pfRandom.h"
#include "pfResample.h"

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (1.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _resampleBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange, uint8_t force);
static void _getRandCdf(float* buf, int count, float s);
static int _floatCmp(const void* a, const void* b);

void pfResample_resample(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    float w, s, ss, csum, ssum, ess, htheta, m;
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
        
        _getRandCdf(randCdf, PF_N_TAG, s);
        
        i = 0;
        j = 0;
        while (i < PF_N_TAG)
        {
            while (i < PF_N_TAG && randCdf[i] < weightCdf[j])
                pfInit_spawnTagParticleFromOther(&tag->pTagBuf[i++], &tag->pTag[j], HXYZ, htheta);
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
    float w, s, ss, ess, m;
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
            _getRandCdf(randCdf, PF_N_BCN, s);
            
            i = 0;
            j = 0;
            while (i < PF_N_BCN)
            {
                while (i < PF_N_BCN && randCdf[i] < weightCdf[j])
                    pfInit_spawnBcnParticleFromOther(&bcn->pBcnBuf[i++], &bcn->pBcn[k][j], HXYZ);
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

static void _getRandCdf(float* buf, int count, float s)
{
    int i;
    
    for (i = 0; i < count; ++i)
        buf[i] = pfRandom_uniform() * s;
    qsort(buf, count, sizeof(buf[0]), _floatCmp);
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
