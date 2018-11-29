//
//  UwbMeasurement.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface UwbMeasurement : NSObject

@property double t;
@property int i;
@property double r;
@property double s;

- (id)initWithT:(double)t i:(int)i r:(double)r s:(double)s;

@end
