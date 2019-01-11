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

    void pfInit_initTag(tag_t* tag);
    void pfInit_initBcn(bcn_t* bcn, const tag_t* tag, float range, float stdRange);
    void pfInit_spawnTagParticle(tagParticle_t* tp);
    void pfInit_spawnBcnParticle(bcnParticle_t* bp, const tagParticle_t* tp, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
