/*
 * pfResample.h
 * Created by John Miller on 11/1/19.
 *
 * Copyright (c) 2019, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _PFRESAMPLE_H
#define _PFRESAMPLE_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfResample_resampleLocFromRange(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfResample_resampleSlamFromRange(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns);
    void pfResample_resampleLocFromPose(particleFilterLoc_t* pf, float x, float y, float z, float theta, float stdXyz, float stdTheta);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
