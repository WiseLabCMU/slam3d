/*
 * test.c
 * Created by Perry Naseck on 6/25/21.
 *
 * Copyright (c) 2021, Wireless Sensing and Embedded Systems Lab, Carnegie
 * Mellon University
 * All rights reserved.
 *
 * This source code is licensed under the BSD-3-Clause license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "particleFilter.h"

// Only one test for now, so keep it simple

#define NUM_BCNS            (4)
#define UWB_STD             (0.1f)
#define UWB_BIAS            (0.2f)
#define SKIP_TO_WAYPOINT    (0)

#define VIO_FILE            "/test1_ParticleFilterLoc_vio.csv"
#define UWB_FILE            "/test1_ParticleFilterLoc_uwb.csv"
#define DEPLOY_FILE         "/test1_ParticleFilterLoc_deploy.csv"
#define TEST_FOLDER         argv[1 + noFail]
#define TAG_OUT_FILE        argv[2 + noFail]
#define EXPECTED_FILE       argv[3 + noFail]
#define LINE_LEN            (1024)
#define SEED                (123456789)

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint);
static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint);
static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3]);
static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta);

static particleFilterLoc_t _particleFilter;

int main(int argc, char** argv) {
  if (argc < 4) {
    printf("Test folder, out file, expected file, and/or --nofail not specified!\n");
    printf("test [--nofail] <test folder> <output file> [expected file (required without --nofail)]\n");
    return 1;
  }

  int noFail = (strcmp(argv[1], "--nofail") == 0);
  char vioFilePath[1024];
  char uwbFilePath[1024];
  char deployFilePath[1024];
  strcpy(vioFilePath, TEST_FOLDER);
  strcpy(uwbFilePath, TEST_FOLDER);
  strcpy(deployFilePath, TEST_FOLDER);
  strcat(vioFilePath, VIO_FILE);
  strcat(uwbFilePath, UWB_FILE);
  strcat(deployFilePath, DEPLOY_FILE);

  printf("Starting test\n");

  particleFilterSeed_set(SEED);

  FILE* vioFile;
  FILE* uwbFile;
  FILE* deployFile;
  FILE* tagOutFile;
  float deployment[NUM_BCNS][3];
  double vioT, uwbT, outT;
  float vioX, vioY, vioZ, uwbR, outX, outY, outZ, outTheta;
  uint8_t uwbB, haveVio, haveUwb;

  clock_t t_measure;

  printf("Starting localization\n");
  vioFile = fopen(vioFilePath, "r");
  uwbFile = fopen(uwbFilePath, "r");
  tagOutFile = fopen(TAG_OUT_FILE, "w");
  particleFilterLoc_init(&_particleFilter);

  deployFile = fopen(deployFilePath, "r");
  _getDeployment(deployFile, deployment);
  fclose(deployFile);

  printf("Initialized\n");

  t_measure = clock();
  haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, SKIP_TO_WAYPOINT);
  haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, SKIP_TO_WAYPOINT);
  while (haveVio || haveUwb) {
    if (haveVio && (!haveUwb || vioT < uwbT)) {
      particleFilterLoc_depositVio(&_particleFilter, vioT, vioX, vioY, vioZ, 0.0f);
      if (particleFilterLoc_getTagLoc(&_particleFilter, &outT, &outX, &outY, &outZ, &outTheta))
        _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta);
      haveVio = _getVio(vioFile, &vioT, &vioX, &vioY, &vioZ, 0);
    } else if (haveUwb) {
      uwbR -= UWB_BIAS;
      if (uwbR > 0.0f && uwbR < 30.0f)
        particleFilterLoc_depositRange(&_particleFilter, deployment[uwbB][0], deployment[uwbB][1], deployment[uwbB][2], uwbR, UWB_STD);
      haveUwb = _getUwb(uwbFile, &uwbT, &uwbB, &uwbR, 0);
    }
  }
  t_measure = clock() - t_measure;
  printf("Finished localization\n");

  double time_taken = ((double)t_measure)/CLOCKS_PER_SEC;
  printf("Loop took %f seconds to execute\n", time_taken);

  fclose(vioFile);
  fclose(uwbFile);
  fclose(tagOutFile);

  FILE* expectedFileCompare;
  FILE* tagOutFileCompare;

  if (argc == 4 && noFail) {
    printf("Expected compare file not provided and called with --nofail, exiting\n");
    return 0;
  }
  
  expectedFileCompare = fopen(EXPECTED_FILE, "r");
  tagOutFileCompare = fopen(TAG_OUT_FILE, "r");
  
  char ch1, ch2;
  int res = 0;
  if (expectedFileCompare == NULL || tagOutFileCompare == NULL) {
    printf("Could not open files to compare!\n");
    if (expectedFileCompare != NULL)
      fclose(expectedFileCompare);
    if (tagOutFileCompare != NULL)
      fclose(tagOutFileCompare);
    return 1;
  }
  while (!feof(expectedFileCompare) && !feof(tagOutFileCompare)) {
    ch1 = fgetc(expectedFileCompare);
    ch2 = fgetc(tagOutFileCompare);
    if (ch1 != ch2) {
      res = 1;
      break;
    }
  }
  if (feof(expectedFileCompare) != feof(tagOutFileCompare)) {
    res = 1;
  }

  fclose(expectedFileCompare);
  fclose(tagOutFileCompare);

  if (!res) {
    printf("Test passed\n");
    return 0;
  } else if (res && noFail) {
    printf("Test failed, but called with --nofail\n");
    return 0;
  } else {
    printf("Test failed\n");
    return 1;
  }
}

static uint8_t _getVio(FILE* vioFile, double* t, float* x, float* y, float* z, uint8_t skipToWaypoint) {
  static char _lineBuf[LINE_LEN];

  if (fgets(_lineBuf, LINE_LEN, vioFile) == NULL)
    return 0;

  *t = atof(strtok(_lineBuf, ","));
  *y = (float)atof(strtok(NULL, ","));    // VIO on iOS is reported in a different order (y, z, x)
  *z = (float)atof(strtok(NULL, ","));
  *x = (float)atof(strtok(NULL, ",\n"));

  return 1;
}

static uint8_t _getUwb(FILE* uwbFile, double* t, uint8_t* b, float* r, uint8_t skipToWaypoint) {
  static char _lineBuf[LINE_LEN];

  if (fgets(_lineBuf, LINE_LEN, uwbFile) == NULL)
    return 0;
  *t = atof(strtok(_lineBuf, ","));
  *b = (uint8_t)atoi(strtok(NULL, ","));
  *r = (float)atof(strtok(NULL, ",\n"));

  assert(*b < NUM_BCNS);
  return 1;
}

static void _getDeployment(FILE* deployFile, float deployment[NUM_BCNS][3]) {
  static char _lineBuf[LINE_LEN];
  int i;
  uint8_t b;

  for (i = 0; i < NUM_BCNS; ++i) {
    if (fgets(_lineBuf, LINE_LEN, deployFile) == NULL)
      return;
    b = (uint8_t)atoi(strtok(_lineBuf, ","));
    assert(b < NUM_BCNS);
    deployment[b][1] = (float)atof(strtok(NULL, ","));
    deployment[b][2] = (float)atof(strtok(NULL, ","));
    deployment[b][0] = (float)atof(strtok(NULL, ",\n"));
  }
}

static void _writeTagLoc(FILE* outFile, double t, float x, float y, float z, float theta) {
  static uint8_t printedHeaders = 0;
  if (!printedHeaders) {
    fprintf(outFile, "t,x,y,z,theta\n");
    printedHeaders = 1;
  }
  fprintf(outFile, "%lf,%f,%f,%f,%f\n", t, y, z, x, theta);
}
