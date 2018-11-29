//
//  Beacon.m
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import "Beacon.h"

@implementation Beacon

- (id)initWithX:(double)x y:(double)y z:(double)z
{
    self = [super init];
    if (self)
    {
        self.x = x;
        self.y = y;
        self.z = z;
    }
    return self;
}

@end
