"""
particlefilter_wrapper.pyx
Created by Perry Naseck on 6/20/21.

Copyright (c) 2021, Wireless Sensing and Embedded Systems Lab, Carnegie
Mellon University
All rights reserved.

This source code is licensed under the BSD-3-Clause license found in the
LICENSE file in the root directory of this source tree.
"""

from libc.stdint cimport uint8_t
import numpy as np
cimport numpy as np

cdef extern from "../include/particleFilter.h":
    ctypedef struct particleFilterLoc_t:
        pass
    void particleFilterSeed_set(unsigned int seed)
    void particleFilterLoc_init(particleFilterLoc_t* pf)
    void particleFilterLoc_depositVio(particleFilterLoc_t* pf, double t, float x, float y, float z, float dist)
    void particleFilterLoc_depositRange(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
    void particleFilterLoc_depositRssi(particleFilterLoc_t* pf, float bx, float by, float bz, int rssi)
    uint8_t particleFilterLoc_getTagLoc(const particleFilterLoc_t* pf, double* t, float* x, float* y, float* z, float* theta)

cdef class ParticleFilterLoc:
    cdef particleFilterLoc_t pf;

    def __cinit__(self) -> None:
        particleFilterLoc_init(&self.pf)

    cpdef void depositVio(self, t: np.float64_t, x: np.float32_t, y: np.float32_t, z: np.float32_t, dist: np.float32_t):
        particleFilterLoc_depositVio(&self.pf, t, x, y, z, dist)

    cpdef void depositRange(self, bx: np.float32_t, by: np.float32_t, bz: np.float32_t, range: np.float32_t, stdRange: np.float32_t):
        particleFilterLoc_depositRange(&self.pf, bx, by, bz, range, stdRange)

    cpdef void depositRssi(self, bx: np.float32_t, by: np.float32_t, bz: np.float32_t, rssi: np.int32_t):
        particleFilterLoc_depositRssi(&self.pf, bx, by, bz, rssi)

    cpdef (uint8_t, np.float64_t, np.float32_t, np.float32_t, np.float32_t, np.float32_t) getTagLoc(self):
        cdef double t;
        cdef float x, y, z, theta;
        cdef uint8_t out = particleFilterLoc_getTagLoc(&self.pf, &t, &x, &y, &z, &theta);
        return (out, t, x, y, z, theta)

cpdef void setSeed(seed: np.uint32):
  particleFilterSeed_set(seed);
