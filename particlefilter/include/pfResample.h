//
//  pfResample.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFRESAMPLE_H
#define _PFRESAMPLE_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfResample_resampleLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfResample_resampleSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
