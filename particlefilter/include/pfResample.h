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
    
    void pfResample_resampleLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfResample_resampleSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
