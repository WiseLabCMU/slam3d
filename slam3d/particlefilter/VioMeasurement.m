//
//  VioMeasurement.m
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import "VioMeasurement.h"

@implementation VioMeasurement

- (id)initWithT:(double)t x:(double)x y:(double)y z:(double)z
{
    self = [super init];
    if (self)
    {
        self.t = t;
        self.x = x;
        self.y = y;
        self.z = z;
    }
    return self;
}

@end
