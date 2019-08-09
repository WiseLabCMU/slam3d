//
//  particleFilter.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PARTICLEFILTER_H
#define _PARTICLEFILTER_H

#include <stdint.h>

#define PF_N_TAG_LOC    (10000)
#define PF_N_TAG_SLAM   (100)
#define PF_N_BCN        (1000)

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
        float w;
        float x;
        float y;
        float z;

    } bcnParticle_t;
    
    typedef struct
    {
        tagParticle_t pTag[PF_N_TAG_LOC];
        tagParticle_t pTagBuf[PF_N_TAG_LOC];
        uint8_t initialized;

    } tagLoc_t;

    typedef struct
    {
        tagParticle_t pTag[PF_N_TAG_SLAM];
        tagParticle_t pTagBuf[PF_N_TAG_SLAM];
        uint8_t initialized;
        
    } tagSlam_t;
    
    typedef struct
    {
        bcnParticle_t pBcn[PF_N_TAG_SLAM][PF_N_BCN];
        bcnParticle_t pBcnBuf[PF_N_BCN];
        uint8_t initialized;
        
    } bcn_t;

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
        tagLoc_t tag;

    } particleFilterLoc_t;

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
        tagSlam_t tag;

    } particleFilterSlam_t;

    void particleFilterLoc_init(particleFilterLoc_t* pf);
    void particleFilterSlam_init(particleFilterSlam_t* pf);
    void particleFilterSlam_addBcn(bcn_t* bcn);
    void particleFilterLoc_depositVio(particleFilterLoc_t* pf, double t, float x, float y, float z, float dist);
    void particleFilterSlam_depositVio(particleFilterSlam_t* pf, double t, float x, float y, float z, float dist);
    void particleFilterLoc_depositRange(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange);
    void particleFilterSlam_depositRange(particleFilterSlam_t* pf, bcn_t* bcn, float range, float stdRange, bcn_t** allBcns, int numBcns);
    void particleFilterLoc_depositRssi(particleFilterLoc_t* pf, float bx, float by, float bz, int rssi);
    void particleFilterSlam_depositRssi(particleFilterSlam_t* pf, bcn_t* bcn, int rssi, bcn_t** allBcns, int numBcns);
    uint8_t particleFilterLoc_getTagLoc(const particleFilterLoc_t* pf, double* t, float* x, float* y, float* z, float* theta);
    uint8_t particleFilterSlam_getTagLoc(const particleFilterSlam_t* pf, double* t, float* x, float* y, float* z, float* theta);
    uint8_t particleFilterSlam_getBcnLoc(const particleFilterSlam_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
