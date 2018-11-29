//
//  OutputLocation.h
//  ParticleFilter
//
//  Created by John Miller on 3/23/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface OutputLocation : NSObject <NSCopying>

@property double t;
@property double x;
@property double y;
@property double z;
@property double theta;
@property double s;

- (id)initWithT:(double)t x:(double)x y:(double)y z:(double)z theta:(double)theta s:(double)s;

@end
