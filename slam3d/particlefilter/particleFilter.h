//
//  particleFilter.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PARTICLEFILTER_H
#define _PARTICLEFILTER_H

#define PF_N_TAG	(100)
#define PF_N_BEACON	(1000)

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		float w;
		float x;
		float y;
		float z;

	} beaconParticle_t;

	typedef struct
	{
		float w;
		float x;
		float y;
		float z;
		float theta;

	} tagParticle_t;

	typedef struct _beacon
	{
        beaconParticle_t (* pBeacon)[PF_N_BEACON];
        beaconParticle_t (* pBeaconTmp)[PF_N_BEACON];
		beaconParticle_t pBeaconBuf1[PF_N_TAG][PF_N_BEACON];
        beaconParticle_t pBeaconBuf2[PF_N_TAG][PF_N_BEACON];
        uint32_t beaconId;
        struct _beacon* nextBeacon;

	} beacon_t;

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
        struct _beacon* firstBeacon;

	} particleFilter_t;

	void particleFilter_init(particleFilter_t* pf);
    void particleFilter_addBeacon(particleFilter_t* pf, beacon_t* b, uint32_t beaconId, float range, float std_range);
	void particleFilter_depositVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float dist);
    void particleFilter_depositUwb(particleFilter_t* pf, uint32_t beaconId, float range, float std_range);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
