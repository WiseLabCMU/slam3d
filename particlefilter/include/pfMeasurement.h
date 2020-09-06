//
//  pfMeasurement.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFMEASUREMENT_H
#define _PFMEASUREMENT_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfMeasurement_applyVioLoc(particleFilterLoc_t* pf, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyVioSlam(particleFilterSlam_t* pf, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyRangeLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfMeasurement_applyRangeSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif
