//
//  ParticleFilter.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Beacon.h"
#import "OutputLocation.h"
#import "Particle.h"
#import "UwbMeasurement.h"
#import "VioMeasurement.h"

@interface ParticleFilter : NSObject

@property BOOL isInit;
@property NSArray* bcn;
@property OutputLocation* loc;
@property VioMeasurement* lastVio;
@property NSMutableArray* particles;
@property NSMutableArray* spareParticles;
@property NSMutableArray* randArray;

- (id)initWithBcn:(NSArray*)bcn;

- (void)depositVio:(VioMeasurement*)vio;
- (void)depositUwb:(UwbMeasurement*)uwb;

- (void)applyVio:(VioMeasurement*)vio;
- (void)applyUwb:(UwbMeasurement*)uwb;
- (void)initializeParticlesFromUwb:(UwbMeasurement*)uwb;
- (void)computeLocationWithT:(double)t;
- (void)resampleParticles;

@end
