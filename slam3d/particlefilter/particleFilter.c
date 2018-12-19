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

static void _initTag(particleFilter_t* pf);
static void _initBeacon(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float std_xyz, float std_theta);
static void _applyUwb(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range);
static void _resampleBeacon(particleFilter_t* pf, beacon_t* b, uint8_t force);

static beacon_t* _getBeacon(const particleFilter_t* pf, uint32_t beaconId);
static int _floatCmp(const void* a, const void* b);
static float _randomUniform(void);
static void _randomNormal2(float* x, float* y);

void particleFilter_init(particleFilter_t* pf)
{
    srand((uint32_t)time(NULL));
    pf->firstBeacon = NULL;
    pf->pTag = pf->pTagBuf1;
    pf->pTagTmp = pf->pTagBuf2;
    _initTag(pf);
}

void particleFilter_addBeacon(particleFilter_t* pf, beacon_t* b, uint32_t beaconId, float range, float std_range)
{
    b->pBeacon = b->pBeaconBuf1;
    b->pBeaconTmp = b->pBeaconBuf2;
    b->beaconId = beaconId;
    b->nextBeacon = pf->firstBeacon;
    pf->firstBeacon = b;
    _initBeacon(pf, b, range, std_range);
}

void particleFilter_depositVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float std_xyz, float std_theta)
{
    _applyVio(pf, dt, dx, dy, dz, std_xyz, std_theta);
}

void particleFilter_depositUwb(particleFilter_t* pf, uint32_t beaconId, float range, float std_range)
{
    beacon_t* b;
    
    b = _getBeacon(pf, beaconId);
    _applyUwb(pf, b, range, std_range);
    _resample(pf, b, range, std_range);
}

static void _initTag(particleFilter_t* pf)
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
}

static void _initBeacon(particleFilter_t* pf, beacon_t* b, float range, float std_range)
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
                rdist = range + 3 * std_range * (_randomUniform() * 2 - 1);
            while (rdist < 0);
            
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
}

static void _applyVio(particleFilter_t* pf, float dt, float dx, float dy, float dz, float std_xyz, float std_theta)
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

static void _applyUwb(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    beaconParticle_t* bp;
    float min_weight, dx, dy, dz, p_range, beacon_sum;
    
    min_weight = (range < 3.0f) ? 0.1f : 0.5f;
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
}

static void _resample(particleFilter_t* pf, beacon_t* b, float range, float std_range)
{
    int i, j;
    tagParticle_t* tp;
    beacon_t* bcn;
    float w, s, ss, csum, ssum, ess, htheta, dx, dy, dz, dtheta, m;
    float weightCdf[PF_N_TAG];
    float randCdf[PF_N_TAG];

    s = 0.0f;
    ss = 0.0f;
    csum = 0.0f;
    ssum = 0.0f;
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
        htheta = csum * csum + ssum * ssum;
        htheta = htheta > 1e-10 ? htheta : 1e-10;
        htheta = htheta < 1 - 1e-10 ? htheta : 1 - 1e-10;
        htheta = sqrtf(-logf(htheta) / ess);
        
        for (i = 0; i < PF_N_TAG; ++i)
            randCdf[i] = _randomUniform() * s;
        qsort(randCdf, PF_N_TAG, sizeof(float), _floatCmp);

        i = 0;
        j = 0;
        while (i < PF_N_TAG)
        {
            while (i < PF_N_TAG && randCdf[i] < weightCdf[j])
            {
                tp = &pf->pTagTmp[i];
                *tp = pf->pTag[j];
                
                _randomNormal2(&dx, &dy);
                _randomNormal2(&dz, &dtheta);
                tp->w = 1.0f;
                tp->x += dx * HXYZ;
                tp->y += dy * HXYZ;
                tp->z += dz * HXYZ;
                tp->theta = fmodf(tp->theta + dtheta * htheta, 2 * (float)M_PI);
                
                ++i;
            }
            ++j;
        }

        tp = pf->pTag;
        pf->pTag = pf->pTagTmp;
        pf->pTagTmp = tp;

        for (bcn = pf->firstBeacon; bcn != NULL; bcn = bcn->nextBeacon)
            _resampleBeacon(pf, bcn, 1);
    }
    else
    {
        m = s / PF_N_TAG;
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
    int num_spawn;
    
    num_spawn = lroundf(PF_N_BEACON * PCT_SPAWN);
    did_spawn = (mean(pBeaconMean(:, :, 1), 2) < WEIGHT_SPAWN_THRESH) & (range < RADIUS_SPAWN_THRESH);
    if any(did_spawn)
        spawn_p = initializeBeaconTrueSLAM(num_spawn, range, stddev, pTag(did_spawn, :), ble_slam);
    end
    beacon_ess = sum(pBeaconMean(:, :, 1), 2).^2 ./ sum(pBeaconMean(:, :, 1).^2, 2);
    needs_resample = (beacon_ess / M < RESAMPLE_THRESH) | did_spawn | force;
    to_resample = pBeaconMean(needs_resample, :, :);
    for j = 1:size(to_resample, 1)
        to_resample(j, :, :) = to_resample(j, randsample(M, M, true, to_resample(j, :, 1)), :);
    to_resample(j, :, 1) = 1;
    to_resample(j, :, 2:4) = to_resample(j, :, 2:4) + HXYZ * randn(size(to_resample(j, :, 2:4)));
    end
    pBeaconMean(needs_resample, :, :) = to_resample;
    if any(did_spawn)
        pBeaconMean(did_spawn, 1:num_spawn, :) = spawn_p;
    end
    pBeaconMean(:, :, 1) = pBeaconMean(:, :, 1) ./ mean(pBeaconMean(:, :, 1), 2);
}

static beacon_t* _getBeacon(const particleFilter_t* pf, uint32_t beaconId)
{
    beacon_t* b;
    for (b = pf->firstBeacon; b != NULL; b = b->nextBeacon)
        if (b->beaconId == beaconId)
            return b;
    return NULL;
}

static int _floatCmp(const void* a, const void* b)
{
    float* x = (float*)a;
    float* y = (float*)b;
    if (*x < *y)
        return -1;
    if (*x > *y)
        return 1;
    return 0;
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
