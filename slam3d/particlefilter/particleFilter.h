//
//  particleFilter.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PARTICLEFILTER_H
#define _PARTICLEFILTER_H

#define PF_N_TAG	(100)
#define PF_N_BCN	(1000)

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		float w;
		float x;
		float y;
		float z;

	} bcnParticle_t;

	typedef struct
	{
		float w;
		float x;
		float y;
		float z;
		float theta;

	} tagParticle_t;

	typedef struct _bcn
	{
        bcnParticle_t (* pBcn)[PF_N_BCN];
        bcnParticle_t (* pBcnTmp)[PF_N_BCN];
		bcnParticle_t pBcnBuf1[PF_N_TAG][PF_N_BCN];
        bcnParticle_t pBcnBuf2[PF_N_TAG][PF_N_BCN];
        uint32_t bcnId;
        struct _bcn* nextBcn;

	} bcn_t;

	typedef struct
	{
        float totalDt;
        float totalDx;
        float totalDy;
        float totalDz;
        float totalDist;
        tagParticle_t* pTag;
        tagParticle_t* pTagTmp;
		tagParticle_t pTagBuf1[PF_N_TAG];
        tagParticle_t pTagBuf2[PF_N_TAG];
        struct _bcn* firstBcn;

	} particleFilter_t;

	void particleFilter_init(particleFilter_t* pf);
    void particleFilter_addBcn(particleFilter_t* pf, bcn_t* b, uint32_t bcnId, float range, float stdRange);
	void particleFilter_depositVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist);
    void particleFilter_depositUwb(particleFilter_t* pf, uint32_t bcnId, float range, float stdRange);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
