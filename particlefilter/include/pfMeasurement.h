/*
 * pfMeasurement.h
 * Created by John Miller on 11/1/19.
 *
 * Copyright (c) 2019, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _PFMEASUREMENT_H
#define _PFMEASUREMENT_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfMeasurement_applyVioLoc(particleFilterLoc_t* pf, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyTagVioSlam(particleFilterSlam_t* pf, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyBcnVioSlam(bcn_t* bcn, float dt, float dx, float dy, float dz, float ddist);
    void pfMeasurement_applyRangeLoc(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfMeasurement_applyRangeSlam(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif
