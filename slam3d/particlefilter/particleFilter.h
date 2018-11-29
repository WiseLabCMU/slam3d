//
//  particleFilter.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PARTICLEFILTER_H
#define _PARTICLEFILTER_H

#include "particleFilter.h"

#define N_TAG		(100)
#define N_BEACON	(1000)

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

	typedef beaconParticle_t _beacon_t[N_TAG][N_BEACON];

	typedef struct
	{
		tagParticle_t pTag[N_TAG];
		beacon_t* pBeacon;

	} _particleFilter_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
