//
//  VioMeasurement.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface VioMeasurement : NSObject

@property double t;
@property double x;
@property double y;
@property double z;

- (id)initWithT:(double)t x:(double)x y:(double)y z:(double)z;

@end
