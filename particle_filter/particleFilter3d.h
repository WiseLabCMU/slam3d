// 
// particleFilter3d.h header file
// 
//

#ifndef PFHEADER
#define PFHEADER

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define PI 3.14159265359

static int NUM_PARTICLES=1000; //global variable indicating the number of particles

typedef struct{
    float w;
    float x;
    float y;
    float z;
    float theta;
    float scale;

}particle_t;

typedef struct{
    float x;
    float y;
    float z;
    float theta;
    float xy_std;
    float z_std;
    float scale;

}location_t;

//********************helper functions to simulate matlab functionality**********

//takes absolute value of float without risking overflow (i.e. doesn't do sqrt(a**2) )
float abs_f(float a);

//takes max of two floats
float max(float a, float b);

//takes min of two floats
float min(float a, float b);

//generates number between [0,1] from uniform distribution
float urand(void);

//********************************particleFilter3d functions *****************************
//randomly samples N integers using weights from set of particles as associated probabilities
//(returns array of integers in the input array int* idcs, by reference)
void randsample(int * idcs, int N, particle_t * p);

//samples from normal distribution with mean mu, and variance sigma using Box-Muller transform
float randn(float mu, float sigma);

//Initializes a set of particles to be used for the particle filtering algorithm (creates array of allocated memory in the heap so must be de-allocated later)
void initializeParticles3d(particle_t* p, int num_particles, float centerx, float centery, float centerz, float radius, float radius_std, float initz, float initz_std, bool constAngle, bool is_rssi); 

//recreates applyUwb3d from matlab
void applyUwb3d(particle_t* p, int num_particles, float centerx, float centery, float centerz, float range,float stddev, bool is_rssi);

//recreates applyVio3d from matlab
void applyVio3d(particle_t* p, int num_particles, float dx, float dy, float dz, float stdev_xyz, float stddev_theta, float stdev_scale);

//generates an estimate of the location from the distribution defined by the set of particles
location_t computeLocation3d(particle_t * p, int num_particles);

//resamples the particles based on modern measurement updates
void resampleParticles3d(particle_t* p, int num_particles, float centerx, float centery, float centerz, float radius, float radius_std, float initz, float initz_std, bool is_rssi);



#endif
