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

#define VIO_FILE    "/Users/johnmiller/Documents/MATLAB/mag_fld_matlab/BuddySLAM/data_CIC/1515283298.129726_vio.csv"
#define UWB_FILE    "/Users/johnmiller/Documents/MATLAB/mag_fld_matlab/BuddySLAM/data_CIC/1515283298.129726_uwb_range.csv"
#define LINE_LEN    (1024)

#define NUM_BCNS    (12)
#define UWB_STD     (0.1f)

static uint8_t _getVio(FILE* vioFile, float* t, float* x, float* y, float* z);
static uint8_t _getUwb(FILE* uwbFile, float* t, uint8_t* b, float* r);

int main()
{
    FILE* vioFile;
    FILE* uwbFile;
    float vioT, vioX, vioY, vioZ;
    float uwbT, uwbR;
    uint8_t uwbB;
	particleFilter_t particleFilter;
    bcn_t bcns[NUM_BCNS];
    uint8_t haveVio, haveUwb;
    
    vioFile = fopen(VIO_FILE, "r");
    uwbFile = fopen(UWB_FILE, "r");
	particleFilter_init(&particleFilter);
    while (1)
    {
        haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ);
        haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR);
        if (haveVio && (!haveUwb || vioT < uwbT))
            particleFilter_depositVio(&particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
        else if (haveUwb)
            particleFilter_depositUwb(&particleFilter, &bcns[uwbB], uwbR, UWB_STD);
        else
            break;
    }
    fclose(vioFile);
    fclose(uwbFile);
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
