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

	typedef struct
	{
		beaconParticle_t pBeacon[PF_N_TAG][PF_N_BEACON];

	} beacon_t;

	typedef struct
	{
		tagParticle_t pTag[PF_N_TAG];

	} particleFilter_t;

	void particleFilter_init(particleFilter_t* pf);
	void particleFilter_applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float std_xyz, float std_theta);
	void particleFilter_addBeacon(particleFilter_t*pf, beacon_t* b);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
