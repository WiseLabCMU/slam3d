//
//  pfLocalize.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PFLOCALIZE_H
#define _PFLOCALIZE_H

#include <stdint.h>

#define PF_N_TAG    (10000)

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct
    {
        float w;
        float x;
        float y;
        float z;
        float theta;
        
    } tagParticle_t;

    typedef struct
    {
        tagParticle_t pTag[PF_N_TAG];
        tagParticle_t pTagBuf[PF_N_TAG];
        
    } tag_t;

    typedef struct
    {
        double firstT;
        float firstX;
        float firstY;
        float firstZ;
        float firstDist;
        double lastT;
        float lastX;
        float lastY;
        float lastZ;
        float lastDist;
        tag_t tag;

    } pfLocalize_t;

    void pfLocalize_init(pfLocalize_t* pf);
    void pfLocalize_depositVio(pfLocalize_t* pf, double t, float x, float y, float z, float dist);
    void pfLocalize_depositRange(pfLocalize_t* pf, float bx, float by, float bz, float range, float stdRange);
    void pfLocalize_depositRssi(pfLocalize_t* pf, float bx, float by, float bz, int rssi);
    void pfLocalize_getTagLoc(const pfLocalize_t* pf, double* t, float* x, float* y, float* z, float* theta);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
