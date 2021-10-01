/*
 * pfRandom.h
 * Created by John Miller on 11/1/19.
 *
 * Copyright (c) 2019, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef _PFRANDOM_H
#define _PFRANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

    extern unsigned int PF_SEED;
    extern int PF_SEED_SET;
    
    void pfRandom_init(void);
    float pfRandom_uniform(void);
    void pfRandom_normal2(float* x, float* y);
    void pfRandom_sphere(float* x, float* y, float* z, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
