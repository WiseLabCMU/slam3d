//
//  pfResample.h
//
//  Created by John Miller on 1/11/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#ifndef _PFRESAMPLE_H
#define _PFRESAMPLE_H

#include "particleFilter.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    void pfResample_resample(tag_t* tag, bcn_t* bcn, bcn_t* firstBcn, float range, float stdRange);
    
#ifdef __cplusplus
} // extern "C"
#endif
    
#endif
