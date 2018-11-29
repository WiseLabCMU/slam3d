//
//  particleFilter.c
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#include "particleFilterInternal.h"

#define SCALE_STD (0.05)

void particleFilter_init(particleFilter_t* pf)
{

}

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


#import "ParticleFilter.h"

#define NUM_PARTICLES   (10000)
#define VIO_XY_STD      (1e-3)
#define VIO_STD_THETA   (1e-6)
#define VIO_STD_SCALE   (1e-3)
#define RESAMPLE_THRESH (0.5)

@implementation ParticleFilter

- (id)initWithBcn:(NSArray*)bcn
{
    self = [super init];
    if (self)
    {
        self.isInit = NO;
        self.bcn = bcn;
        self.loc = [[OutputLocation alloc] initWithT:0 x:0 y:0 z:0 theta:0 s:0];
        self.lastVio = [[VioMeasurement alloc] initWithT:0 x:0 y:0 z:0];
        self.particles = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
        self.spareParticles = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
        self.randArray = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
        for (int i = 0; i < NUM_PARTICLES; ++i)
        {
            [self.particles addObject:[[Particle alloc] init]];
            [self.spareParticles addObject:[[Particle alloc] init]];
            [self.randArray addObject:[NSNumber numberWithDouble:0]];
        }
    }
    return self;
}

- (void)depositVio:(VioMeasurement*)vio
{
    [self applyVio:vio];
    [self computeLocationWithT:vio.t];
}

- (void)depositUwb:(UwbMeasurement*)uwb
{
    if (!self.isInit)
        [self initializeParticlesFromUwb:uwb];
    else
        [self applyUwb:uwb];
    
    [self resampleParticles];
    [self computeLocationWithT:uwb.t];
}

- (void)applyVio:(VioMeasurement *)vio
{
    uint32_t r1[NUM_PARTICLES];
    uint32_t r2[NUM_PARTICLES];
    uint32_t r3[NUM_PARTICLES];
    uint32_t r4[NUM_PARTICLES];
    
    arc4random_buf(r1, sizeof(r1));
    arc4random_buf(r2, sizeof(r2));
    arc4random_buf(r3, sizeof(r3));
    arc4random_buf(r4, sizeof(r4));
    
    double dt = vio.t - self.lastVio.t;
    double dx = vio.x - self.lastVio.x;
    double dy = vio.y - self.lastVio.y;
    double s_dt = sqrt(dt);
    double s_dxy = sqrt(hypot(dx, dy));
    
    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        Particle* p = self.particles[i];
        double theta = p.theta;
        double scale = p.scale;
        double c = cos(theta);
        double s = sin(theta);
        
        double f1 = sqrt(-2 * log((double)r1[i] / UINT32_MAX));
        double g1 = sqrt(-2 * log((double)r2[i] / UINT32_MAX));
        double f2 = (double)r3[i] / UINT32_MAX * 2 * M_PI;
        double g2 = (double)r4[i] / UINT32_MAX * 2 * M_PI;
        
        p.x += scale * (dx * c - dy * s) + f1 * cos(f2) * VIO_XY_STD * s_dxy;
        p.y += scale * (dx * s + dy * c) + f1 * sin(f2) * VIO_XY_STD * s_dxy;
        theta += g1 * cos(g2) * VIO_STD_THETA * s_dt;
        p.scale += g1 * sin(g2) * VIO_STD_SCALE * s_dt;
        
        p.theta = fmod(theta, 2 * M_PI);
    }
    self.lastVio = vio;
}

- (void)applyUwb:(UwbMeasurement*)uwb
{
    Beacon* b = self.bcn[uwb.i];
    for (Particle* p in self.particles)
        if (fabs(hypot(p.x - b.x, p.y - b.y) - uwb.r) > 3 * uwb.s)
            p.w *= 0.5;
}

- (void)initializeParticlesFromUwb:(UwbMeasurement*)uwb
{
    for (Particle* p in self.particles)
        [p initFromUwb:uwb beacon:self.bcn[uwb.i]];
    
    self.isInit = YES;
}

- (void)computeLocationWithT:(double)t
{
    if (!self.isInit)
        return;
    
    double wsum = 0;
    double xsum = 0;
    double ysum = 0;
    double csum = 0;
    double ssum = 0;
    for (Particle* p in self.particles)
    {
        double w = p.w;
        wsum += w;
        xsum += w * p.x;
        ysum += w * p.y;
        csum += w * cos(p.theta);
        ssum += w * sin(p.theta);
    }
    self.loc.t = t;
    self.loc.x = xsum / wsum;
    self.loc.y = ysum / wsum;
    self.loc.z = self.lastVio.z;
    self.loc.theta = atan2(ssum, csum);
    
    double dsum = 0;
    for (Particle* p in self.particles)
    {
        double dx = p.x - self.loc.x;
        double dy = p.y - self.loc.y;
        dsum += p.w * (dx * dx + dy * dy);
    }
    self.loc.s = sqrt(dsum / wsum);
}

- (void)resampleParticles
{
    double s = 0;
    double ss = 0;
    double csum = 0;
    double ssum = 0;
    double scaleSum = 0;
    double weightCdf[NUM_PARTICLES];
    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        Particle* p = self.particles[i];
        double w = p.w;
        s += w;
        ss += w * w;
        csum += w * cos(p.theta);
        ssum += w * sin(p.theta);
        scaleSum += w * p.scale;
        weightCdf[i] = s;
    }
    double ess = s * s / ss;
    
    if (ess / NUM_PARTICLES < RESAMPLE_THRESH)
    {
        csum /= s;
        ssum /= s;
        scaleSum /= s;
        
        double scaleDev = 0;
        for (int i = 0; i < NUM_PARTICLES; ++i)
        {
            Particle* p = self.particles[i];
            self.randArray[i] = [NSNumber numberWithDouble:(double)arc4random() / UINT32_MAX * s];
            scaleDev += p.w * (p.scale - scaleSum) * (p.scale - scaleSum);
        }
        NSArray* randCdf = [self.randArray sortedArrayUsingSelector:@selector(compare:)];
        scaleDev /= s;
        
        double hXY = 0.1;
        double hTheta = sqrt(-log(csum * csum + ssum * ssum) / ess);
        double hScale = sqrt(scaleDev / sqrt(ess));
        
        int i = 0;
        int j = 0;
        int k = 0;
        while (k < NUM_PARTICLES)
        {
            while (k < NUM_PARTICLES && [randCdf[i] doubleValue] < weightCdf[j])
            {
                [self.spareParticles[k++] initFromOther:self.particles[j]
                                        withXYBandwidth:hXY
                                     withThetaBandwidth:hTheta
                                     withScaleBandwidth:hScale];
                ++i;
            }
            ++j;
        }
        
        NSMutableArray* tmp = self.particles;
        self.particles = self.spareParticles;
        self.spareParticles = tmp;
    }
    else
    {
        double m = s / NUM_PARTICLES;
        for (Particle* p in self.particles)
            p.w /= m;
    }
}

@end
