//
//  slam3d.c
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#include <stdio.h>

#include "particlefilter/particleFilter.h"

int main()
{
	beacon_t beacons[10];
	particleFilter_t particleFilter;

	particleFilter_init(particleFilter);

	printf("Hello, World!\n");

	return 0;
}
