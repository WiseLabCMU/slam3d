//
//  csvslam.c
//
//  Created by John Miller on 11/1/18.
//  Copyright � 2018 CMU. All rights reserved.
//

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "particleFilter.h"

#define DATA_DIR            "../sampledata/"
#define TRACE_DIR           DATA_DIR "cic/0/"
#define NUM_BCNS            (12)
#define UWB_STD             (0.1f)
#define UWB_BIAS            (0.4f)
#define SKIP_TO_WAYPOINT    (1)

#define VIO_FILE            TRACE_DIR "vio.csv"
#define UWB_FILE            TRACE_DIR "uwb.csv"
#define TAG_OUT_FILE        TRACE_DIR "tag.csv"
#define BCN_OUT_FILE        TRACE_DIR "bcn.csv"
#define LINE_LEN            (1024)

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint);
static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint);
static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta);
static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z, float theta);

static particleFilterSlam_t _particleFilter;
static bcn_t _bcns[NUM_BCNS];
static bcn_t *_bcnPtrs[NUM_BCNS];

int main(int argc, char** argv)
{
    FILE* vioFile;
    FILE* uwbFile;
    FILE* tagOutFile;
    FILE* bcnOutFile;
    double vioT, uwbT, outT;
    float vioX, vioY, vioZ, uwbR, outX, outY, outZ, outTheta;
    uint8_t uwbB, haveVio, haveUwb;
    int i;
    
    printf("Starting localization\n");
    vioFile = fopen(VIO_FILE, "r");
    uwbFile = fopen(UWB_FILE, "r");
    tagOutFile = fopen(TAG_OUT_FILE, "w");
    bcnOutFile = fopen(BCN_OUT_FILE, "w");
    particleFilterSlam_init(&_particleFilter);
    for (i = 0; i < NUM_BCNS; ++i)
    {
        particleFilterSlam_addBcn(&_bcns[i]);
        _bcnPtrs[i] = &_bcns[i];
    }
    printf("Initialized\n");
    
    haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, SKIP_TO_WAYPOINT);
    haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, SKIP_TO_WAYPOINT);
    while (haveVio || haveUwb)
    {
        if (haveVio && (!haveUwb || vioT < uwbT))
        {
            particleFilterSlam_depositTagVio(&_particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
            if (particleFilterSlam_getTagLoc(&_particleFilter, &outT, &outX, &outY, &outZ, &outTheta))
                _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta);
            haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, 0);
        }
        else if (haveUwb)
        {
            uwbR -= UWB_BIAS;
            if (uwbR > 0.0f && uwbR < 30.0f)
                particleFilterSlam_depositRange(&_particleFilter, &_bcns[uwbB], uwbR, UWB_STD, _bcnPtrs, NUM_BCNS);
            haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, 0);
        }
    }
    printf("Finished localization\n");
    for (uwbB = 0; uwbB < NUM_BCNS; ++uwbB)
    {
        if (particleFilterSlam_getBcnLoc(&_particleFilter, &_bcns[uwbB], &outT, &outX, &outY, &outZ, &outTheta))
            _writeBcnLoc(bcnOutFile, uwbB, outX, outY, outZ, outTheta);
    }

    fclose(vioFile);
    fclose(uwbFile);
    fclose(tagOutFile);
    fclose(bcnOutFile);
    
    printf("Done\n");
    return 0;
}

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint)
{
    static char _lineBuf[LINE_LEN];
    char waypoint;

    do
    {
        if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
            return 0;
        *t = atof(strtok(_lineBuf, ","));
        strtok(NULL, ","); // Skip "position" or "orientation" string
        waypoint = strtok(NULL, ",")[0]; // Skip waypoint number
        strtok(NULL, ","); // Skip accuracy number
        *y = (float)atof(strtok(NULL, ","));    // VIO on iOS is reported in a different order (y, z, x)
        *z = (float)atof(strtok(NULL, ","));
        *x = (float)atof(strtok(NULL, ",\n"));
        fgets(_lineBuf, LINE_LEN, vioFile); // Skip line for orientation
    } while (skipToWaypoint && waypoint < '4');
    
    return 1;
}

static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint)
{
    static char _lineBuf[LINE_LEN];
    char waypoint;

    do
    {
        if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
            return 0;
        *t = atof(strtok(_lineBuf, ","));
        strtok(NULL, ","); // Skip "uwb_range" string
        waypoint = strtok(NULL, ",")[0]; // Skip waypoint number
        *b = strtok(NULL, ",")[0] - 'a';
        *r = (float)atof(strtok(NULL, ",\n"));
    } while (skipToWaypoint && waypoint < '4');
    
    assert(*b < NUM_BCNS);
    return 1;
}

static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta)
{
    static uint8_t printedHeaders = 0;
    if (!printedHeaders)
    {
        fprintf(outFile, "t,x,y,z,theta\n");
        printedHeaders = 1;
    }
    fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, x, y, z, theta);
}

static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z, float theta)
{
    static uint8_t printedHeaders = 0;
    if (!printedHeaders)
    {
        fprintf(outFile, "b,x,y,z,theta\n");
        printedHeaders = 1;
    }
    fprintf(outFile, "%hhu,%f,%f,%f,%f\n", b, x, y, z, theta);
}
