#!/usr/bin/env python
"""
test.py
Created by Perry Naseck on 6/25/21.

Copyright (c) 2021, Wireless Sensing and Embedded Systems Lab, Carnegie
Mellon University
All rights reserved.

This source code is licensed under the BSD-3-Clause license found in the
LICENSE file in the root directory of this source tree.
"""

import csv
from filecmp import cmp
import numpy as np
import sys
import time
from particlefilter import ParticleFilterLoc, setSeed

# Only one test for now, so keep it simple

NUM_BCNS = 4
UWB_STD  = 0.1
UWB_BIAS = 0.2
SKIP_TO_WAYPOINT = 0

VIO_FILE       = "/test1_ParticleFilterLoc_vio.csv"
UWB_FILE       = "/test1_ParticleFilterLoc_uwb.csv"
DEPLOY_FILE    = "/test1_ParticleFilterLoc_deploy.csv"
LINE_LEN       = 1024
SEED           = 123456789

ALLOW_VARIANCE = 0.000005

printedHeaders = False

def _getVio(vioFile, skipToWaypoint: int) -> tuple:
  _lineBuf = vioFile.readline(LINE_LEN)
  if (_lineBuf == ""):
    return (False, None, None, None, None)
  split = (_lineBuf[:-1]).split(",")
  t = np.float64(split[0])
  y = np.float32(split[1])    # VIO on iOS is reported in a different order (y, z, x)
  z = np.float32(split[2])
  x = np.float32(split[3])

  return (True, t, x, y, z)

def _getUwb(uwbFile, skipToWaypoint: int) -> tuple:
  _lineBuf = uwbFile.readline(LINE_LEN)
  if (_lineBuf == ""):
    return (False, None, None, None)
  split = (_lineBuf[:-1]).split(",")
  t = np.float64(split[0])
  b = np.int32(split[1])
  r = np.float32(split[2])

  assert(b < NUM_BCNS)
  return (True, t, b, r)

def _getDeployment(deployFile, deployment):
  for i in range(NUM_BCNS):
    _lineBuf = deployFile.readline(LINE_LEN)
    if (_lineBuf == ""):
      return
    split = (_lineBuf[:-1]).split(",") # remove last char (newline)
    b = int(split[0])
    assert(b < NUM_BCNS)
    deployment[b][1] = np.float32(split[1])
    deployment[b][2] = np.float32(split[2])
    deployment[b][0] = np.float32(split[3])

def _writeTagLoc(outFile, t: np.float64, x: np.float32, y: np.float32, z: np.float32, theta: np.float32):
  outFile.write(("{:.6f},{:.6f},{:.6f},{:.6f},{:.6f}\n").format(t, y, z, x, theta))

def main() -> int:
  if len(sys.argv) < 4:
    print("Test folder, out file, expected file, and/or --nofail not specified!")
    print("test.py [--nofail] <test folder> <output file> [expected file (required without --nofail)]")
    return 1
  
  noFail = 0
  if sys.argv[1] == "--nofail":
    noFail = 1
  testFolder = sys.argv[1 + noFail]
  tagOutFilePath = sys.argv[2 + noFail]

  print("Starting test")

  setSeed(SEED)

  print("Starting localization")
  vioFile = open(testFolder + VIO_FILE, 'r')
  uwbFile = open(testFolder + UWB_FILE, 'r')
  tagOutFile = open(tagOutFilePath, 'w')
  tagOutFile.write("t,x,y,z,theta\n")
  _particleFilter = ParticleFilterLoc()

  deployFile = open(testFolder + DEPLOY_FILE, 'r')
  deployment = [[np.float32(0)]*3 for i in range(NUM_BCNS)]
  _getDeployment(deployFile, deployment)
  deployFile.close()

  print("Initialized")

  t_measure = time.perf_counter()
  haveVio, vioT, vioX, vioY, vioZ = _getVio(vioFile, SKIP_TO_WAYPOINT)
  haveUwb, uwbT, uwbB, uwbR = _getUwb(uwbFile, SKIP_TO_WAYPOINT)
  while haveVio or haveUwb:
    if haveVio and (not haveUwb or vioT < uwbT):
      _particleFilter.depositVio(vioT, vioX, vioY, vioZ, 0.0)
      status, outT, outX, outY, outZ, outTheta = _particleFilter.getTagLoc()
      if status:
        _writeTagLoc(tagOutFile, outT, outX, outY, outZ, outTheta)
      haveVio, vioT, vioX, vioY, vioZ = _getVio(vioFile, 0)
    elif haveUwb:
      uwbR -= UWB_BIAS
      if uwbR > 0.0 and uwbR < 30.0:
        _particleFilter.depositRange(deployment[uwbB][0], deployment[uwbB][1], deployment[uwbB][2], uwbR, UWB_STD)
      haveUwb, uwbT, uwbB, uwbR = _getUwb(uwbFile, 0)
  t_measure = time.perf_counter() - t_measure
  print("Finished localization")

  print(f"Loop took {t_measure:.6f} seconds to execute")

  vioFile.close()
  uwbFile.close()
  tagOutFile.close()

  if len(sys.argv) == 2 and sys.argv[1] == "--nofail":
    print("Expected compare file not provided and called with --nofail, exiting")
    return 0
  
  expectedFilePath = sys.argv[3 + noFail]

  res = cmp(expectedFilePath, tagOutFilePath, shallow=False)
  if res:
    print("Test passed, exact match")
    return 0
  else:
    print(f"Test not exact match, checking all numbers within {ALLOW_VARIANCE}")

  res = 1
  num_off = 0
  num_exact = 0
  num_allowable = 0
  max_allowable_off_found = 0.0
  max_off_found = 0.0
  file_length = "same number as in expected"
  
  csvExpectedFile = open(expectedFilePath, 'r', newline='')
  csvTagOutFile = open(tagOutFilePath, 'r', newline='')

  expectedFileReader = list(csv.reader(csvExpectedFile, delimiter=','))
  tagOutFileReader = list(csv.reader(csvTagOutFile, delimiter=','))

  # flatten lists from rows into one big list
  expectedParams = np.array(expectedFileReader).flatten()
  tagOutParams = np.array(tagOutFileReader).flatten()
  paramsIter = expectedParams

  if len(expectedParams) > len(tagOutParams):
    file_length = "fewer parameters than expected"
    paramsIter = tagOutParams
    res = 0
  elif len(expectedParams) < len(tagOutParams):
    file_length = "more parameters than expected"
    res = 0

  for i in range(5, len(paramsIter)): # skip first row (header)
    diff = abs(float(expectedParams[i]) - float(tagOutParams[i]))
    if expectedParams[i] == tagOutParams[i]:
      num_exact += 1
    elif diff <= ALLOW_VARIANCE:
      num_allowable += 1
      if diff > max_allowable_off_found:
        max_allowable_off_found = diff
    else:
      num_off += 1
      if diff > max_off_found:
        max_off_found = diff
  
  if num_off > 0:
    res = 0

  print(f"Number of parameters in output file: {file_length}")
  print(f"Number of parameters out of range: {num_off}")
  print(f"Number of parameters exact match: {num_exact}")
  print(f"Number of parameters within allowable range (not exact match): {num_allowable}")
  print(f"Maximum difference found in out of range: {max_off_found:.8f}")
  print(f"Maximum difference found in allowable: {max_allowable_off_found:.8f}")

  if res:
    print("Test passed, within allowed deviation")
    return 0
  elif not res and len(sys.argv) > 1 and sys.argv[1] == "--nofail":
    print("Test failed, but called with --nofail")
    return 0
  else:
    print("Test failed")
    return 1

if __name__=="__main__":
   res = main()
   exit(res)
