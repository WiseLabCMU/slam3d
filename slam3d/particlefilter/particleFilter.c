//
//  particleFilter.c
//
//  Created by John Miller on 11/1/18.
//  Copyright © 2018 CMU. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <math.h>
#undef _USE_MATH_DEFINES
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "particleFilter.h"

#define RESAMPLE_THRESH     (0.5f)
#define RADIUS_SPAWN_THRESH (1.0f)
#define WEIGHT_SPAWN_THRESH (0.4f)
#define PCT_SPAWN           (0.05f)
#define HXYZ                (0.1f)

static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _resampleBeacon(particleFilter_t* pf, beacon_t* b, uint8_t force);

static float _randomUniform(void);
static void _randomNormal2(float* x, float* y);

void particleFilter_init(particleFilter_t* pf)
{
	int i;
	tagParticle_t* tp;

	for (i = 0; i < PF_N_TAG; ++i)
	{
		tp = &pf->pTag[i];
		tp->w = 1.0f;
		tp->x = 0.0f;
		tp->y = 0.0f;
		tp->z = 0.0f;
		tp->theta = 0.0f;
	}
    pf->firstBeacon = NULL;
    
	srand((uint32_t)time(NULL));
}

void particleFilter_addBeacon(particleFilter_t* pf, beacon_t* b, uint32_t beaconId, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    beaconParticle_t* bp;
    float rdist, relev, razim;
    float c, dx, dy, dz;
    
    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        for (j = 0; j < PF_N_BEACON; ++j)
        {
            bp = &b->pBeacon[i][j];
            
            do
            {
                rdist = range + 3 * std_range * (_randomUniform() * 2 - 1);
            } while (rdist < 0);
            
            relev = asinf(_randomUniform() * 2 - 1);
            razim = _randomUniform() * 2 * (float)M_PI;
            
            c = rdist * cosf(relev);
            dx = c * cosf(razim);
            dy = c * sinf(razim);
            dz = rdist * sinf(relev);
            
            bp->w = 1.0f;
            bp->x = tp->x + dx;
            bp->y = tp->y + dy;
            bp->z = tp->z + dz;
        }
    }
    b->beaconId = beaconId;
    b->nextBeacon = pf->firstBeacon;
    pf->firstBeacon = b;
}

void particleFilter_applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float std_xyz, float std_theta)
{
	int i;
	tagParticle_t* tp;
	float c, s, p_dx, p_dy;
	float rx, ry, rz, rtheta;

	for (i = 0; i < PF_N_TAG; ++i)
	{
		tp = &pf->pTag[i];
		c = cosf(tp->theta);
		s = sinf(tp->theta);
		p_dx = dx * c - dy * s;
		p_dy = dx * s + dy * c;

		_randomNormal2(&rx, &ry);
		_randomNormal2(&rz, &rtheta);

		tp->x += p_dx + std_xyz * rx;
		tp->y += p_dy + std_xyz * ry;
		tp->z += dz + std_xyz * rz;
		tp->theta = fmodf(tp->theta + std_theta * rtheta, 2 * (float)M_PI);
	}
}

void particleFilter_applyUwb(particleFilter_t* pf, uint32_t beaconId, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    beaconParticle_t* bp;
    beacon_t* b;
    float min_weight, dx, dy, dz, p_range, beacon_sum;
    
    min_weight = (range < 3.0f) ? 0.1f : 0.5f;
    for (b = pf->firstBeacon; b != NULL; b = b->nextBeacon)
    {
        if (b->beaconId == beaconId)
        {
            for (i = 0; i < PF_N_TAG; ++i)
            {
                tp = &pf->pTag[i];
                beacon_sum = 0.0f;
                for (j = 0; j < PF_N_BEACON; ++j)
                {
                    bp = &b->pBeacon[i][j];
                    dx = tp->x - bp->x;
                    dy = tp->y - bp->y;
                    dz = tp->z - bp->z;
                    p_range = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (fabsf(p_range - range) > 3 * std_range)
                    {
                        bp->w *= min_weight;
                        beacon_sum += bp->w;
                    }
                }
                tp->w *= beacon_sum;
            }
            break;
        }
    }
}

static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i;
    tagParticle_t* tp;
    float w, s, ss, csum, ssum, ess;
    float weightCdf[PF_N_TAG];
    float randArray[PF_N_TAG];

    for (i = 0; i < PF_N_TAG; ++i)
    {
        tp = &pf->pTag[i];
        w = tp->w;
        s += w;
        ss += w * w;
        csum += w * cosf(tp->theta);
        ssum += w * sinf(tp->theta);
        weightCdf[i] = s;
    }
    ess = s * s / ss;
    
    if (ess / PF_N_TAG < RESAMPLE_THRESH)
    {
        csum /= s;
        ssum /= s;
        for (i = 0; i < PF_N_TAG; ++i)
        {
            tp = &pf->pTag[i];
            randArray[i] = _randomUniform() * s;
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
        
        indices = randsample(N, N, true, pTag(:, 1));
        pTag = pTag(indices, :);
        temp = mean(cos(pTag(:, 4)))^2 + mean(sin(pTag(:, 4)))^2;
        temp = max(temp, 0.000001);
        temp = min(temp, 0.999999);
        devt = sqrt(-log(temp));
        ht = devt * ess^-.5;
        pTag(:, 1) = 1;
        pTag(:, 2:4) = pTag(:, 2:4) + HXYZ * randn(size(pTag(:, 2:4)));
        pTag(:, 5) = pTag(:, 5) + ht * randn(size(pTag(:, 5)));
        pTag(:, 5) = mod(pTag(:, 5), 2 * pi);
        for i = 1:length(pBeaconsMean)
        {
            if numel(pBeaconsMean{i}) > 0
            {
                [pBeaconsMean{i}, pBeaconsCov{i}] = resampleBeacon(pTag, pBeaconsMean{i}(indices, :, :), pBeaconsCov{i}(indices, :, :, :), true);
            }
        }
    }
//    Otherwise, normalize top layer and resample lower layers if necessary
    else
    {
        float m = s / PF_N_TAG;
        for (i = 0; i < PF_N_TAG; ++i)
        {
            tp = &pf->pTag[i];
            tp->w /= m;
        }
        _resampleBeacon(pf, b, 0);
    }
}

static void _resampleBeacon(particleFilter_t* pf, beacon_t* b, uint8_t force)
{
    
}

static float _randomUniform(void)
{
	return (float)rand() / RAND_MAX;
}

static void _randomNormal2(float* x, float* y)
{
	float f = sqrtf(-2 * logf(_randomUniform()));
	float g = _randomUniform() * 2 * (float)M_PI;
	*x = f * cosf(g);
	*y = f * sinf(g);
}

//- (void)initFromOther:(Particle*)other withXYBandwidth:(double)hXY withThetaBandwidth:(double)hTheta withScaleBandwidth:(double)hScale
//{
//	double f1 = sqrt(-2 * log((double)arc4random() / UINT32_MAX));
//	double g1 = sqrt(-2 * log((double)arc4random() / UINT32_MAX));
//	double f2 = (double)arc4random() / UINT32_MAX * 2 * M_PI;
//	double g2 = (double)arc4random() / UINT32_MAX * 2 * M_PI;
//	
//	self.w = 1.0;
//	self.x = other.x + f1 * cos(f2) * hXY;
//	self.y = other.y + f1 * sin(f2) * hXY;
//	self.theta = other.theta + g1 * cos(g2) * hTheta;
//	self.scale = other.scale + g1 * sin(g2) * hScale;
//	
//	self.theta = fmod(self.theta, 2 * M_PI);
//}
//
//
//#define NUM_PARTICLES   (10000)
//#define VIO_XY_STD      (1e-3)
//#define VIO_STD_THETA   (1e-6)
//#define VIO_STD_SCALE   (1e-3)
//#define RESAMPLE_THRESH (0.5)
//
//- (id)initWithBcn:(NSArray*)bcn
//{
//	self = [super init];
//	if (self)
//	{
//		self.isInit = NO;
//		self.bcn = bcn;
//		self.loc = [[OutputLocation alloc] initWithT:0 x:0 y:0 z:0 theta:0 s:0];
//		self.lastVio = [[VioMeasurement alloc] initWithT:0 x:0 y:0 z:0];
//		self.particles = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
//		self.spareParticles = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
//		self.randArray = [NSMutableArray arrayWithCapacity:NUM_PARTICLES];
//		for (int i = 0; i < NUM_PARTICLES; ++i)
//		{
//			[self.particles addObject:[[Particle alloc] init]];
//			[self.spareParticles addObject:[[Particle alloc] init]];
//			[self.randArray addObject:[NSNumber numberWithDouble:0]];
//		}
//	}
//	return self;
//}
//
//- (void)depositVio:(VioMeasurement*)vio
//{
//	[self applyVio:vio];
//	[self computeLocationWithT:vio.t];
//}
//
//- (void)depositUwb:(UwbMeasurement*)uwb
//{
//	if (!self.isInit)
//		[self initializeParticlesFromUwb:uwb];
//	else
//		[self applyUwb:uwb];
//	
//	[self resampleParticles];
//	[self computeLocationWithT:uwb.t];
//}
//
//
//- (void)initializeParticlesFromUwb:(UwbMeasurement*)uwb
//{
//	for (Particle* p in self.particles)
//		[p initFromUwb:uwb beacon:self.bcn[uwb.i]];
//	
//	self.isInit = YES;
//}
//
//- (void)computeLocationWithT:(double)t
//{
//	if (!self.isInit)
//		return;
//	
//	double wsum = 0;
//	double xsum = 0;
//	double ysum = 0;
//	double csum = 0;
//	double ssum = 0;
//	for (Particle* p in self.particles)
//	{
//		double w = p.w;
//		wsum += w;
//		xsum += w * p.x;
//		ysum += w * p.y;
//		csum += w * cos(p.theta);
//		ssum += w * sin(p.theta);
//	}
//	self.loc.t = t;
//	self.loc.x = xsum / wsum;
//	self.loc.y = ysum / wsum;
//	self.loc.z = self.lastVio.z;
//	self.loc.theta = atan2(ssum, csum);
//	
//	double dsum = 0;
//	for (Particle* p in self.particles)
//	{
//		double dx = p.x - self.loc.x;
//		double dy = p.y - self.loc.y;
//		dsum += p.w * (dx * dx + dy * dy);
//	}
//	self.loc.s = sqrt(dsum / wsum);
//}
//
//- (void)resampleParticles
//{
//	double s = 0;
//	double ss = 0;
//	double csum = 0;
//	double ssum = 0;
//	double scaleSum = 0;
//	double weightCdf[NUM_PARTICLES];
//	for (int i = 0; i < NUM_PARTICLES; ++i)
//	{
//		Particle* p = self.particles[i];
//		double w = p.w;
//		s += w;
//		ss += w * w;
//		csum += w * cos(p.theta);
//		ssum += w * sin(p.theta);
//		scaleSum += w * p.scale;
//		weightCdf[i] = s;
//	}
//	double ess = s * s / ss;
//	
//	if (ess / NUM_PARTICLES < RESAMPLE_THRESH)
//	{
//		csum /= s;
//		ssum /= s;
//		scaleSum /= s;
//		
//		double scaleDev = 0;
//		for (int i = 0; i < NUM_PARTICLES; ++i)
//		{
//			Particle* p = self.particles[i];
//			self.randArray[i] = [NSNumber numberWithDouble:(double)arc4random() / UINT32_MAX * s];
//			scaleDev += p.w * (p.scale - scaleSum) * (p.scale - scaleSum);
//		}
//		NSArray* randCdf = [self.randArray sortedArrayUsingSelector:@selector(compare:)];
//		scaleDev /= s;
//		
//		double hXY = 0.1;
//		double hTheta = sqrt(-log(csum * csum + ssum * ssum) / ess);
//		double hScale = sqrt(scaleDev / sqrt(ess));
//		
//		int i = 0;
//		int j = 0;
//		int k = 0;
//		while (k < NUM_PARTICLES)
//		{
//			while (k < NUM_PARTICLES && [randCdf[i] doubleValue] < weightCdf[j])
//			{
//				[self.spareParticles[k++] initFromOther:self.particles[j]
//										withXYBandwidth:hXY
//									 withThetaBandwidth:hTheta
//									 withScaleBandwidth:hScale];
//				++i;
//			}
//			++j;
//		}
//		
//		NSMutableArray* tmp = self.particles;
//		self.particles = self.spareParticles;
//		self.spareParticles = tmp;
//	}
//	else
//	{
//		double m = s / NUM_PARTICLES;
//		for (Particle* p in self.particles)
//			p.w /= m;
//	}
//}
