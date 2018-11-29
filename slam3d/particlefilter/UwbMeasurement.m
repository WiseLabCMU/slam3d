//
//  UwbMeasurement.m
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import "UwbMeasurement.h"

@implementation UwbMeasurement

- (id)initWithT:(double)t i:(int)i r:(double)r s:(double)s
{
    self = [super init];
    if (self)
    {
        self.t = t;
        self.i = i;
        self.r = r;
        self.s = s;
    }
    return self;
}

@end
