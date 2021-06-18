slam3d
======
Simultaneous localization and mapping (SLAM) tools in 3D

Jump to: [C](#c), [Python Wrapper](#python-wrapper)

## C

Source found in `./particlefilter`.

### Shared library
```
gcc -fPIC -shared -o particlefilter.so -lm -Iparticlefilter/include particlefilter/src/*.c
```

## Python Wrapper

Source found in `./particlefilter/cython`. Currently implements `particleFilterLoc`. Clone and run `python3 -m pip3 install .` to build and install.

```python3
from particleFilter import ParticleFilterLoc

pf = ParticleFilterLoc()

pf.depositVio(t, x, y, z, dist)
pf.depositRange(bx, by, bz, range, stdRange)
pf.depositRssi(bx, by, bz, rssi)
pf.getTagLoc() # returns tuple: (t, x, y, z, theta)

```


