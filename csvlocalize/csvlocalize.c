//
//  csvlocalize.c
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

//#define DATA_DIR            "../sampledata/"
//#define TRACE_DIR           DATA_DIR "cic/0/"
//#define NUM_BCNS            (12)
//#define UWB_STD             (0.1f)
//#define UWB_BIAS            (0.4f)
//#define SKIP_TO_WAYPOINT    (1)
//
//#define VIO_FILE            TRACE_DIR "vio.csv"
//#define UWB_FILE            TRACE_DIR "uwb.csv"
//#define DEPLOY_FILE         TRACE_DIR "deploy.csv"
//#define TAG_OUT_FILE        TRACE_DIR "tag.csv"
//#define LINE_LEN            (1024)

#define DATA_DIR            "../mqttlogger/"
#define TRACE_DIR           DATA_DIR
#define NUM_BCNS            (4)
#define UWB_STD             (0.1f)
#define UWB_BIAS            (0.2f)
#define SKIP_TO_WAYPOINT    (0)

#define VIO_FILE            TRACE_DIR "vio.csv"
#define UWB_FILE            TRACE_DIR "uwb.csv"
#define DEPLOY_FILE         TRACE_DIR "../sampledata/arena/deploy.csv"
#define TAG_OUT_FILE        TRACE_DIR "tag.csv"
#define LINE_LEN            (1024)

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint);
static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint);
static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3]);
static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta);

static particleFilterLoc_t _particleFilter;

int main(int argc, char** argv)
{
    FILE* vioFile;
    FILE* uwbFile;
    FILE* deployFile;
    FILE* tagOutFile;
    float deployment[NUM_BCNS][3];
    double vioT, uwbT, outT;
    float vioX, vioY, vioZ, uwbR, outX, outY, outZ, outTheta;
    uint8_t uwbB, haveVio, haveUwb;

    printf("Starting localization\n");
    vioFile = fopen(VIO_FILE, "r");
    uwbFile = fopen(UWB_FILE, "r");
    tagOutFile = fopen(TAG_OUT_FILE, "w");
    particleFilterLoc_init(&_particleFilter);

    deployFile = fopen(DEPLOY_FILE, "r");
    _getDeployment(deployFile, deployment);
    fclose(deployFile);

    printf("Initialized\n");

    haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, SKIP_TO_WAYPOINT);
    haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, SKIP_TO_WAYPOINT);
    while (haveVio || haveUwb)
    {
        if (haveVio && (!haveUwb || vioT < uwbT))
        {
            particleFilterLoc_depositVio(&_particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
            if (particleFilterLoc_getTagLoc(&_particleFilter, &outT, &outX, &outY, &outZ, &outTheta))
                _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta);
            haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, 0);
        }
        else if (haveUwb)
        {
            uwbR -= UWB_BIAS;
            if (uwbR > 0.0f && uwbR < 30.0f)
                particleFilterLoc_depositRange(&_particleFilter, deployment[uwbB][0], deployment[uwbB][1], deployment[uwbB][2], uwbR, UWB_STD);
            haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, 0);
        }
    }
    printf("Finished localization\n");

    fclose(vioFile);
    fclose(uwbFile);
    fclose(tagOutFile);

    printf("Done\n");
    return 0;
}

//static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint)
//{
//    static char _lineBuf[LINE_LEN];
//    char waypoint;
//
//    do
//    {
//        if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
//            return 0;
//        *t = atof(strtok(_lineBuf, ","));
//        strtok(NULL, ","); // Skip "position" or "orientation" string
//        waypoint = strtok(NULL, ",")[0]; // Skip waypoint number
//        strtok(NULL, ","); // Skip accuracy number
//        *y = (float)atof(strtok(NULL, ","));    // VIO on iOS is reported in a different order (y, z, x)
//        *z = (float)atof(strtok(NULL, ","));
//        *x = (float)atof(strtok(NULL, ",\n"));
//        fgets(_lineBuf, LINE_LEN, vioFile); // Skip line for orientation
//    } while (skipToWaypoint && waypoint < '4');
//
//    return 1;
//}

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint)
{
    static char _lineBuf[LINE_LEN];

    if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
        return 0;

    *t = atof(strtok(_lineBuf, ","));
    *y = (float)atof(strtok(NULL, ","));    // VIO on iOS is reported in a different order (y, z, x)
    *z = (float)atof(strtok(NULL, ","));
    *x = (float)atof(strtok(NULL, ",\n"));

    return 1;
}

//static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint)
//{
//    static char _lineBuf[LINE_LEN];
//    char waypoint;
//
//    do
//    {
//        if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
//            return 0;
//        *t = atof(strtok(_lineBuf, ","));
//        strtok(NULL, ","); // Skip "uwb_range" string
//        waypoint = strtok(NULL, ",")[0]; // Skip waypoint number
//        *b = strtok(NULL, ",")[0] - 'a';
//        *r = (float)atof(strtok(NULL, ",\n"));
//    } while (skipToWaypoint && waypoint < '4');
//
//    assert(*b < NUM_BCNS);
//    return 1;
//}

static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint)
{
    static char _lineBuf[LINE_LEN];

    if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
        return 0;
    *t = atof(strtok(_lineBuf, ","));
    *b = (uint8_t)atoi(strtok(NULL, ","));
    *r = (float)atof(strtok(NULL, ",\n"));

    assert(*b < NUM_BCNS);
    return 1;
}

static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3])
{
    static char _lineBuf[LINE_LEN];
    int i;
    uint8_t b;

    for (i = 0; i < NUM_BCNS; ++i)
    {
        if (fgets(_lineBuf, LINE_LEN, deployFile) == NULL)
            return;
        b = (uint8_t)atoi(strtok(_lineBuf, ","));
        assert(b < NUM_BCNS);
        deployment[b][1] = (float)atof(strtok(NULL, ","));
        deployment[b][2] = (float)atof(strtok(NULL, ","));
        deployment[b][0] = (float)atof(strtok(NULL, ",\n"));
    }
}

//static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta)
//{
//    static uint8_t printedHeaders = 0;
//    if (!printedHeaders)
//    {
//        fprintf(outFile, "t,x,y,z,theta\n");
//        printedHeaders = 1;
//    }
//    fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, x, y, z, theta);
//}

static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta)
{
    static uint8_t printedHeaders = 0;
    if (!printedHeaders)
    {
        fprintf(outFile, "t,x,y,z,theta\n");
        printedHeaders = 1;
    }
    fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, y, z, x, theta);
}
