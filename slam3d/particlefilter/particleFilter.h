//
//  particleFilter.h
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#define N_TAG		(100)
#define N_BEACON	(1000)

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

typedef beaconParticle_t beacon_t[N_TAG][N_BEACON];

typedef struct
{
	tagParticle_t pTag[N_TAG];
	beacon_t* pBeacon;

} particleFilter_t;

