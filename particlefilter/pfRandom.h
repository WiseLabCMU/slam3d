//
//  pfRandom.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFRANDOM_H
#define _PFRANDOM_H

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfRandom_init(void);
    float pfRandom_uniform(void);
    void pfRandom_normal2(float* x, float* y);
    void pfRandom_sphere(float* x, float* y, float* z, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
