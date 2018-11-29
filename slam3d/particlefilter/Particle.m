//
//  Particle.m
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import "Particle.h"

#define SCALE_STD (0.05)

@implementation Particle

- (void)initFromUwb:(UwbMeasurement*)uwb beacon:(Beacon*)beacon
{
    double angle = (double)arc4random() / UINT32_MAX * 2 * M_PI;
    double dist = ((double)arc4random() / UINT32_MAX * 2 - 1) * 3 * uwb.s + uwb.r;
    
    double f1 = sqrt(-2 * log((double)arc4random() / UINT32_MAX));
    double f2 = (double)arc4random() / UINT32_MAX * 2 * M_PI;
    
    self.w = 1.0;
    self.x = beacon.x + dist * cos(angle);
    self.y = beacon.y + dist * sin(angle);
    self.theta = (double)arc4random() / UINT32_MAX * 2 * M_PI;
    self.scale = f1 * cos(f2) * SCALE_STD + 1;
}

- (void)initFromOther:(Particle*)other withXYBandwidth:(double)hXY withThetaBandwidth:(double)hTheta withScaleBandwidth:(double)hScale
{
    double f1 = sqrt(-2 * log((double)arc4random() / UINT32_MAX));
    double g1 = sqrt(-2 * log((double)arc4random() / UINT32_MAX));
    double f2 = (double)arc4random() / UINT32_MAX * 2 * M_PI;
    double g2 = (double)arc4random() / UINT32_MAX * 2 * M_PI;
    
    self.w = 1.0;
    self.x = other.x + f1 * cos(f2) * hXY;
    self.y = other.y + f1 * sin(f2) * hXY;
    self.theta = other.theta + g1 * cos(g2) * hTheta;
    self.scale = other.scale + g1 * sin(g2) * hScale;
    
    self.theta = fmod(self.theta, 2 * M_PI);
}

@end
