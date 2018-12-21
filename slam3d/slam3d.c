//
//  slam3d.c
//
//  Created by John Miller on 11/1/18.
//  Copyright � 2018 CMU. All rights reserved.
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "particlefilter/particleFilter.h"

#define VIO_FILE        "/Users/johnmiller/Documents/MATLAB/mag_fld_matlab/BuddySLAM/data_CIC/1515283298.129726_vio.csv"
#define UWB_FILE        "/Users/johnmiller/Documents/MATLAB/mag_fld_matlab/BuddySLAM/data_CIC/1515283298.129726_uwb_range.csv"
#define TAG_OUT_FILE    "/Users/johnmiller/Desktop/tag.csv"
#define BCN_OUT_FILE    "/Users/johnmiller/Desktop/bcn.csv"
#define LINE_LEN        (1024)

#define NUM_BCNS        (12)
#define UWB_STD         (0.1f)
#define UWB_BIAS        (0.4f)

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z);
static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r);
static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta);
static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z);

static particleFilter_t _particleFilter;
static bcn_t _bcns[NUM_BCNS];

int main(void)
{
    FILE* vioFile;
    FILE* uwbFile;
    FILE* tagOutFile;
    FILE* bcnOutFile;
    double vioT, uwbT, outT;
    float vioX, vioY, vioZ, uwbR, outX, outY, outZ, outTheta;
    uint8_t uwbB, haveVio, haveUwb;
    int v, u;
    
    printf("Starting localization\n");
    vioFile = fopen(VIO_FILE, "r");
    uwbFile = fopen(UWB_FILE, "r");
    tagOutFile = fopen(TAG_OUT_FILE, "w");
    bcnOutFile = fopen(BCN_OUT_FILE, "w");
    particleFilter_init(&_particleFilter);
    printf("Initialized\n");
    
    v = 0;
    u = 0;
    haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ);
    haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR);
    while (haveVio || haveUwb)
    {
        if (haveVio && (!haveUwb || vioT < uwbT))
        {
            particleFilter_depositVio(&_particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
            particleFilter_getTagLoc(&_particleFilter, &outT, &outX, &outY, &outZ, &outTheta);
            _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta);
            haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ);
            printf("Applied VIO %d\n", v++);
        }
        else if (haveUwb)
        {
            uwbR -= UWB_BIAS;
            if (uwbR > 0.0f && uwbR < 30.0f)
                particleFilter_depositUwb(&_particleFilter, &_bcns[uwbB], uwbR, UWB_STD);
            haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR);
            printf("Applied UWB %d\n", u++);
        }
    }
    printf("Finished localization\n");
    for (uwbB = 0; uwbB < NUM_BCNS; ++uwbB)
    {
        particleFilter_getBcnLoc(&_particleFilter, &_bcns[uwbB], &outT, &outX, &outY, &outZ);
        _writeBcnLoc(bcnOutFile, uwbB, outX, outY, outZ);
        printf("Wrote beacon output\n");
    }

    fclose(vioFile);
    fclose(uwbFile);
    fclose(tagOutFile);
    fclose(bcnOutFile);
    
    printf("Done\n");
	return 0;
}

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z)
{
    static char _lineBuf[LINE_LEN];

    if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
        return 0;
    *t = atof(strtok(_lineBuf, ","));
    strtok(NULL, ","); // Skip "position" or "orientation" string
    strtok(NULL, ","); // Skip waypoint number
    strtok(NULL, ","); // Skip accuracy number
    *y = (float)atof(strtok(NULL, ","));    // VIO on iOS is reported in a different order (y, z, x)
    *z = (float)atof(strtok(NULL, ","));
    *x = (float)atof(strtok(NULL, ",\n"));
    fgets(_lineBuf, LINE_LEN, vioFile); // Skip line for orientation
    return 1;
}

static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r)
{
    static char _lineBuf[LINE_LEN];

    if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
        return 0;
    *t = atof(strtok(_lineBuf, ","));
    strtok(NULL, ","); // Skip "uwb_range" string
    strtok(NULL, ","); // Skip waypoint number
    *b = strtok(NULL, ",")[0] - 'a';
    *r = (float)atof(strtok(NULL, ",\n"));
    
    assert(*b < NUM_BCNS);
    return 1;
}

static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta)
{
    fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, x, y, z, theta);
}

static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z)
{
    fprintf(outFile, "%hhu,%f,%f,%f\n", b, x, y, z);
}
