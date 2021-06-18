from libc.stdint cimport uint8_t

cdef extern from "../include/particleFilter.h":
    ctypedef struct particleFilterLoc_t:
        pass
    void particleFilterLoc_init(particleFilterLoc_t* pf)
    void particleFilterLoc_depositVio(particleFilterLoc_t* pf, double t, float x, float y, float z, float dist)
    void particleFilterLoc_depositRange(particleFilterLoc_t* pf, float bx, float by, float bz, float range, float stdRange)
    void particleFilterLoc_depositRssi(particleFilterLoc_t* pf, float bx, float by, float bz, int rssi)
    uint8_t particleFilterLoc_getTagLoc(const particleFilterLoc_t* pf, double* t, float* x, float* y, float* z, float* theta)

cdef class ParticleFilterLoc:
    cdef particleFilterLoc_t pf;

    def __cinit__(self) -> None:
        particleFilterLoc_init(&self.pf)

    cpdef void depositVio(self, t: float, x: float, y: float, z: float, dist: float):
        particleFilterLoc_depositVio(&self.pf, t, x, y, z, dist)

    cpdef void depositRange(self, bx: float, by: float, bz: float, range: float, stdRange: float):
        particleFilterLoc_depositRange(&self.pf, bx, by, bz, range, stdRange)

    cpdef void depositRssi(self, bx: float, by: float, bz: float, rssi: int):
        particleFilterLoc_depositRssi(&self.pf, bx, by, bz, rssi)

    cpdef (double, float, float, float, float) getTagLoc(self):
        cdef double t;
        cdef float x, y, z, theta;
        particleFilterLoc_getTagLoc(&self.pf, &t, &x, &y, &z, &theta)
        return (t, x, y, z, theta)
