//
//  OutputLocation.m
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import "OutputLocation.h"

@implementation OutputLocation

- (id)initWithT:(double)t x:(double)x y:(double)y z:(double)z theta:(double)theta s:(double)s
{
    self = [super init];
    if (self)
    {
        self.t = t;
        self.x = x;
        self.y = y;
        self.z = z;
        self.theta = theta;
        self.s = s;
    }
    return self;
}

- (id)copyWithZone:(NSZone*)zone
{
    OutputLocation *copy = [[[self class] allocWithZone: zone] init];
    copy.t = self.t;
    copy.x = self.x;
    copy.y = self.y;
    copy.z = self.z;
    copy.theta = self.theta;
    copy.s = self.s;
    return copy;
}

@end
