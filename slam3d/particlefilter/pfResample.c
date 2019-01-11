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

void pfResample_resample(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange)
{
    int i, j;
    tagParticle_t* tp;
    float invN, w, s, ss, csum, ssum, ess, htheta, m, rStart, rStep;
    float weightCdf[PF_N_TAG];
    
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
    
    invN = 1.0f / PF_N_TAG;
    if (ess * invN < RESAMPLE_THRESH)
    {
        csum /= s;
        ssum /= s;
        htheta = csum * csum + ssum * ssum;
        htheta = htheta > 1e-10 ? htheta : 1e-10;
        htheta = htheta < 1 - 1e-10 ? htheta : 1 - 1e-10;
        htheta = sqrtf(-logf(htheta) / ess);
        
        rStep = invN * s;
        rStart = pfRandom_uniform() * rStep;
        
        for (i = 0, j = 0; i < PF_N_TAG; ++j)
            for (; i < PF_N_TAG && (rStart + rStep * i) < weightCdf[j]; ++i)
                pfInit_spawnTagParticleFromOther(&tag->pTagBuf[i], &tag->pTag[j], HXYZ, htheta);
        
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
    float invN, w, s, ss, ess, m, rStart, rStep;
    float weightCdf[PF_N_BCN];
    
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
        
        invN = 1.0f / PF_N_BCN;
        numSpawn = 0;
        if (s * invN < WEIGHT_SPAWN_THRESH && range < RADIUS_SPAWN_THRESH)
            numSpawn = (int)lroundf(PF_N_BCN * PCT_SPAWN);
        
        if (ess * invN < RESAMPLE_THRESH || numSpawn > 0 || force)
        {
            rStep = invN * s;
            rStart = pfRandom_uniform() * rStep;
            
            for (i = 0, j = 0; i < PF_N_BCN; ++j)
                for (; i < PF_N_BCN && (rStart + rStep * i) < weightCdf[j]; ++i)
                    pfInit_spawnBcnParticleFromOther(&bcn->pBcnBuf[i], &bcn->pBcn[k][j], HXYZ);
            
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
