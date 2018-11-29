//
//  Particle.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Beacon.h"
#import "UwbMeasurement.h"

@interface Particle : NSObject

@property double w;
@property double x;
@property double y;
@property double theta;
@property double scale;

- (void)initFromUwb:(UwbMeasurement*)uwb beacon:(Beacon*)beacon;
- (void)initFromOther:(Particle*)other withXYBandwidth:(double)hXY withThetaBandwidth:(double)hTheta withScaleBandwidth:(double)hScale;

@end
