//
// implementation of C functions for doing particle filtering using uwb and vio
//

#include "particleFilter3d.h"
#include <time.h>

//takes absolute value of float, without risking overflow
float abs_f(float a){
    return a>=0 ? a : a-2*a;
}

//returns max between two floats
float max (float a, float b){
    return a > b ? a : b;
}

//returns min between two floats
float min(float a, float b){
    return a < b ? a : b;
}

//generates random float between 0 and 1
float urand(void){
    time_t t;
    srand((unsigned) time(&t));
    return rand() * (1.0/RAND_MAX);
}

//samples a random number from 1-N with a vector of weights coming from a set of particles
void randsample(int * idcs, int N, particle_t * p){
    float sum_weights, rnd;
    //initialize memory for a vector so that you can do binary search
    float * ws = malloc(sizeof(float)*N);
    //get sum of weights
    for(int i=0;i<N;i++){
        sum_weights += p[i].w;
        ws[i] = sum_weights;
    }
    
  
    int lo, mid, hi;
    for(int d=0;d<N;d++){
        lo=0;
        hi=N; 
        
        //generate random number between 0 andsum_weights
        rnd = urand()*sum_weights; 
        
        //do binary search to find which "bin" the random number is in
        while(lo!=hi){
            mid = (lo+hi)/2;
            if(rnd>=ws[mid]){ 
                lo = mid+1;
            }
            else{ // if(rnd<ws[idx]){
                hi = mid;
            }
        }
        //now lo==hi, and both point to the proper element
        idcs[d]=lo;
    }

    free(ws);
}


//samples from a normal distribution using the Box-Muller Transform
float randn(float mu, float sigma){
    time_t t;
    srand((unsigned) time(*t)); //seed pseudo-random number generator
    //get two samples from uniform distribution (that are greater than zero so that log()!=->infinity)
    do{
        u1 = rand() * (1.0/ RAND_MAX);
        u2 = rand() * (1.0/ RAND_MAX);
    }while(u1 <= 1.1754943508E-38); //FLT_MIN
    //apply the box muller transform to go from uniform to independent normal
    float z0, z1;
    z0 = sqrt(-2*log(u1)) * cos(2*PI*u2);
    z1 = sqrt(-2*log(u1)) * sin(2*PI*u2);
    //return first number from normal arbitrarily (optimization would be to use global flag to use other value)
    return z0*sigma + mu;
}


// Initializes a set of particles to be used for filtering (NOTE: this function initializes the particles but it does not allocate them. The calling function must allocate the particles
// so that the particles can be allocated statically.)
//  Inputs:
//  int num_particles - number of particles currently being initialized (could be different than the global number of particles since this function is sometimes used to create temporary sets of particles)
//  float centerx, float centery, float centerz - the x,y,z, coordinates of the center respectively
//  float radius - radius of points
//  float radius_std - standard deviation of radius
//  float initz - 
//  float initz-std - 
//  float constAngle - 
//  bool is_rssi - boolean indicating whether the input is an rssi measurement 
void initializeParticles3d(particle_t * p, int num_particles, float centerx, float centery, float centerz, float radius, float radius_std, float initz, float initz_std, bool constAngle, bool is_rssi){
    float z, minR, maxR, dist, angle;

    //loop through and initialize each particle
    for(int i=0; i<num_particles; i++){
        p[i].w = 1; //initialize weight to one
        
        p[i].z = initz + ( (urand()*2)-1 )*3*initz_std;
        
        minR = sqrt( max(pow(radius-(3*radius_std),2) - pow(z-centerz,2), 0) );
        maxR = sqrt( max(pow(radius+(3*radius_std),2) - pow(z-centerz,2), 0) );
        angle = (urand())*2*PI;
        dist = minR + (urand()*(maxR-minR));

        if(is_rssi){
            dist = urand()*3;
        }

        p[i].x = centerx + dist*cos(angle);
        p[i].y = centery + dist*sin(angle);

        p[i].theta = urand()*2*PI;

        if(constAngle){
            p[i].theta = 0;
        }

        p[i].scale = randn(0.05,1);       
    }

    return;
} 

// Makes an update on the particle weights given a uwb range
// Inputs:
// particle_t* p  -array of particles
// float centerx, float centery, float centerz - the x,y,z coordinates of the center
// float range - the uwb range measurement
// float float stddev - the standard-deviation of the range estimate?
// bool is_rssi - whether this was an rssi measurement
void applyUwb3d(particle_t* p, int num_particles, float centerx, float centery, float centerz, float range,float stddev, bool is_rssi){
    float p_range;
    float MIN_WEIGHT = 0.5;
    if range < 3{
        MIN_WEIGHT=0.1;
    }
    
    for(int i=0; i<num_particles; i++){
        p_range = sqrt( pow(p[i].x-centerx,2) + pow(p[i].y-centery ,2) + pow(p[i].z-centerz ,2) );
        if(!is_rssi){
            if(abs_f(p_range-range) > 3*stddev ){
                p[i].w = p[i].w*MIN_WEIGHT;
            }    
        }
        else{
            if(p_range > 3){  //originally had abs_f(p_range), but this is unnecessary because p_range must be positive
                p[i].w = p[i].w*MIN_WEIGHT;
            }
        }
    } 
    
}

// Makes an update to the particles given a vio measurement
// Inputs:
// particle_t* p - set of particles
// ... ?
void applyVio3d(particle_t* p, int num_particles, float dx, float dy, float dz, float stdev_xyz, float stddev_theta, float stdev_scale){
    float c, s, p_dx, p_dy, p_dz;

    for (int i=0; i<num_particles; i++){
        //get parameters
        c = cos(p[i].theta);
        s = sin(p[i].theta);
        p_dx = dx*c - dy*s;
        p_dy = dx*s + dy*c;
        p_dz = dz*1;
        
        //update the x,y,z location of particles
        p[i].x += (p[i].scale*p_dx) + randn(0,stddev_xyz);
        p[i].y += (p[i].scale*p_dy) + randn(0,stddev_xyz);
        p[i].z += (p[i].scale*p_dz) + randn(0,stddev_xyz);
        //update the theta of particles
        p[i].theta += randn(0,stddev_theta);
        p[i].theta = p[i].theta % (2*PI);
        //update the scale of particles
        p[i].scale += randn(0,stddev_scale);
    }
}

//gets the location of the robot given the distribution defined by the set of particles
location_t computeLocation3d(particle_t * p, int num_particles){
    location_t loc;
    float W=0, x=0, y=0, z=0, cossum=0, sinsum=0, xy_std=0, z_std=0, scale =0;
    float pw,px,py,pz;
    //compute all cumulative sums 
    for(int i=0; i<num_particles; i++){
        pw = p[i].w; px=p[i].x; py=[i].y; pz=p[i].z;
        pt = p[i].theta; ps = p[i].scale;

        W += pw;
        x += pw*px;
        y += pw*py;
        z += pw*pz;

        cossum += (pw * cos(pt));
        sinsum += (pw * sin(pt));
        
        xy_std += (pw * (pow(px-x,2) +pow(py-y,2)));
        z_std  += (pw * pow(pz-z,2));
        scale  += (pw * ps); 
    }  
    //get the actual final values 
    loc.x = x / W;
    loc.y = y / W;
    loc.z = z / W;
    loc.theta = atan2(sinsum, cossum);
    loc.xy_std = sqrt(xy_std / W);
    loc.z_std  = sqrt(z_std / W);
    loc.scale = scale / W;

    return loc;
}

//resamples the particles
void resampleParticles3d(particle_t* p, int num_particles, float centerx, float centery, float centerz, float radius, float radius_std, float initz, float initz_std, bool is_rssi){
    int N  = num_particles;
    float RESAMPLE_THRESH = 0.5;
    float RADIUS_SPAWN_THRESH = 1;
    float WEIGHT_SPAWN_THRESH = 0.4;
    int NUM_SPAWN = 500;
    bool didSpawn = false;
    int idx;
    //create copy of particle array which is necessary to resample with replacement
    particle_t * p_copy[N];
    //get any cumulative sums
    float mean_weight = 0, ess=0, p=0, ps=0, mcos=0, msin=0, mscale=0, scalesq=0;
    for(int i=0;i<N;i++){
        mean_weight += p[i].w;
        mcos += cos(p[i].theta);
        msin += sin(p[i].theta);
        mscale += p[i].scale;
        scalesq += pow(p[i].scale,2);
        p += p[i].w;
        ps += pow(p[i].w,2);
        p_copy[i].w=p[i].w; p_copy[i].x=p[i].x; p_copy[i].y=p[i].y; p_copy[i].z=p[i].z; p_copy[i].theta=p[i].theta; p_copy[i].scale=p[i].scale;
    }
    mean_weight = mean_weight / N;
    mcos = mcos / N;
    msin = msin / N;
    mscale = mscale / N;
    ess = pow(p,2)/ps;

    if((mean_weight < SPAWN_THRESHOLD) && (radius < RADIUS_SPAWN_THRESHOLD)){
        //allocate array of temporarily spawned particles
        particle_t * spawn_p[NUM_SPAWN]; 
        //initialize the particles
        initializeParticles3d(&spawn_p,NUM_SPAWN,centerx,centery,centerz,radius radius_std,initz,initz_sd,false,is_rssi);
        didSpawn = true; 
    }
    
    //create array of indices for resampling
    int * idcs[NUM_SPAWN];
    randsample(idcs,N,p); //pass indices into array idcs by reference
    //get parameters: 
    int h = 0.1;
    float devt = sqrt(-log(pow(mcos,2) + pow(msin,2)));
    float ht = devt*pow(ess,-0.5);
    float devs = sqrt((scalesq/N) - pow(mscale,2)); // == std(p.scale)
    float hs = devs*pow(ess,-0.25);
        
    //do the actual resampling
    if((ess/N)<RESAMPLE_THRESH || didSpawn){
        for(int d=0;d<N;d++){
           idx = idcs[d];
           p[d].w=p_copy[idx].w; p[d].x=p_copy[idx].x; p[d].y=p_copy[idx].y; p[d].z=p_copy[idx].z; p[d].theta=p_copy[idx].theta; p[d].scale=p_copy[idx].scale;
           if(didSpawn){
               p[d].w=spawn_p[d].w; p[d].x=spawn_p[d].x; p[d].y=spawn_p[d].y; p[d].z=spawn_p[d].z; p[d].theta=spwan_p[d].theta; p[d].scale=spawn_p[d].scale;
           }
           p[d].w=1;
           p[d].x += randn(0,h);
           p[d].y += randn(0,h);
           p[d].z += randn(0,h);
           p[d].theta += randn(0,ht);
           p[d].theta = p[d].theta % (2*PI);
           p[d].scale += randn(0,hs);
        }      
    }
    else{
        for(int d=0;d<N;d++){
            p[i].w = p[i].w/mean_weights;
        }
    } 
}







