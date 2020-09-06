//
//  pfInit.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFINIT_H
#define _PFINIT_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

    void pfInit_initTagLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfInit_initTagSlam(particleFilterSlam_t* pf);
    void pfInit_initBcnSlam(bcn_t* bcn, const particleFilterSlam_t* pf, float range, float stdRange);
    void pfInit_spawnTagParticleZero(tagParticle_t* tp);
    void pfInit_spawnTagParticleFromRange(tagParticle_t* tp, float bx, float by, float bz, float range, float stdRange);
    void pfInit_spawnTagParticleFromOther(tagParticle_t* tp, const tagParticle_t* other, float hXyz, float hTheta);
    void pfInit_spawnBcnParticleFromRange(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange);
    void pfInit_spawnBcnParticleFromOther(bcnParticle_t* bp, const bcnParticle_t* other, float hXyz, float hTheta);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
