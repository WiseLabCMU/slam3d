//
//  pfInit.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFINIT_H
#define _PFINIT_H

#include "pfSlam.h"

#ifdef __cplusplus
extern "C" {
#endif

    void pfInit_initTag(tag_t* tag, float bx, float by, float bz, float range, float stdRange);
    void pfInit_initTagSlam(tag_t* tag);
    void pfInit_initBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange);
    void pfInit_spawnTagParticle(tagParticle_t* tp);
    void pfInit_spawnTagParticleFromRange(tagParticle_t* tp, float bx, float by, float bz, float range, float stdRange);
    void pfInit_spawnTagParticleFromOther(tagParticle_t* tp, const tagParticle_t* other, float hXyz, float hTheta);
    void pfInit_spawnBcnParticleFromRange(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange);
    void pfInit_spawnBcnParticleFromOther(bcnParticle_t* bp, const bcnParticle_t* other, float hXyz);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
