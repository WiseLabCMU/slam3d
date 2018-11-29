//
//  particleFilter.h
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#ifndef _PARTICLEFILTER_H
#define _PARTICLEFILTER_H

#ifdef __cplusplus
extern "C" {
#endif

	typedef _beacon_t beacon_t;
	typedef _particleFilter_t particleFilter_t;

	void particleFilter_init(particleFilter_t* pf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
