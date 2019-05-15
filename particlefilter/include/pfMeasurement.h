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
    
    void pfMeasurement_applyVio(tag_t* tag, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyRange(tag_t* tag, bcn_t* bcn, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif
