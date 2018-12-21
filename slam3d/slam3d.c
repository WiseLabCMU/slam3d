//
//  slam3d.c
//
//  Created by John Miller on 11/1/18.
//  Copyright � 2018 CMU. All rights reserved.
//

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

static uint8_t _getVio(FILE* vioFile, float* t, float* x, float* y, float* z);
static uint8_t _getUwb(FILE* uwbFile, float* t, uint8_t* b, float* r);
static void _writeTagLoc(FILE* outFile, float t, float x, float y, float z, float theta);
static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z);

int main()
{
    FILE* vioFile;
    FILE* uwbFile;
    FILE* tagOutFile;
    FILE* bcnOutFile;
    float vioT, vioX, vioY, vioZ;
    float uwbT, uwbR;
    uint8_t uwbB;
    float outT, outX, outY, outZ, outTheta;
	particleFilter_t particleFilter;
    bcn_t bcns[NUM_BCNS];
    uint8_t haveVio, haveUwb;
    
    vioFile = fopen(VIO_FILE, "r");
    uwbFile = fopen(UWB_FILE, "r");
    tagOutFile = fopen(TAG_OUT_FILE, "w");
    bcnOutFile = fopen(BCN_OUT_FILE, "w");
	particleFilter_init(&particleFilter);
    
    haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ);
    haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR);
    while (haveVio || haveUwb)
    {
        if (haveVio && (!haveUwb || vioT < uwbT))
        {
            particleFilter_depositVio(&particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
            particleFilter_getTagLoc(&particleFilter, &outT, &outX, &outY, &outZ, &outTheta);
            _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta);
            haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ);
        }
        else if (haveUwb)
        {
            particleFilter_depositUwb(&particleFilter, &bcns[uwbB], uwbR, UWB_STD);
            haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR);
        }
    }
    for (uwbB = 0; uwbB < NUM_BCNS; ++uwbB)
    {
        particleFilter_getBcnLoc(&particleFilter, &bcns[uwbB], &outT, &outX, &outY, &outZ);
        _writeBcnLoc(bcnOutFile, uwbB, outX, outY, outZ);
    }
    
    fclose(vioFile);
    fclose(uwbFile);
    fclose(tagOutFile);
    fclose(bcnOutFile);
    
	return 0;
}

static uint8_t _getVio(FILE* vioFile, float* t, float* x, float* y, float* z)
{
    static char _lineBuf[LINE_LEN];
    
    if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
        return 0;
    *t = (float)atof(strtok(_lineBuf, ","));
    strtok(NULL, ","); // Skip "position" or "orientation" string
    strtok(NULL, ","); // Skip waypoint number
    strtok(NULL, ","); // Skip accuracy number
    *x = (float)atof(strtok(NULL, ","));
    *y = (float)atof(strtok(NULL, ","));
    *z = (float)atof(strtok(NULL, ",\n"));
    fgets(_lineBuf, LINE_LEN, vioFile); // Skip line for orientation
    return 1;
}

static uint8_t _getUwb(FILE* uwbFile, float* t, uint8_t* b, float* r)
{
    static char _lineBuf[LINE_LEN];
    
    if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
        return 0;
    *t = (float)atof(strtok(_lineBuf, ","));
    strtok(NULL, ","); // Skip "uwb_range" string
    strtok(NULL, ","); // Skip waypoint number
    *b = strtok(NULL, ",")[0] - 'a';
    *r = (float)atof(strtok(NULL, ",\n"));
    return 1;
}

static void _writeTagLoc(FILE* outFile, float t, float x, float y, float z, float theta)
{
    fprintf(outFile, "%f,%f,%f,%f,%f\n", t, x, y, z, theta);
}

static void _writeBcnLoc(FILE* outFile, uint8_t b, float x, float y, float z)
{
    fprintf(outFile, "%hhu,%f,%f,%f\n", b, x, y, z);
}
