//
//  Beacon.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Beacon : NSObject

@property double x;
@property double y;
@property double z;

- (id)initWithX:(double)x y:(double)y z:(double)z;

@end
