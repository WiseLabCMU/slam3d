slam3d
======
Simultaneous localization and mapping (SLAM) tools in 3D

## Python Wrapper

Currently implements `particleFilterLoc`. Supports Python 3.6+.

### Installation

Install the [slam3d package from PyPi](https://pypi.org/project/slam3d/):

```
python3 -m pip install slam3d
```
Or use the source distribution or wheels in the [latest release](https://github.com/WiseLabCMU/slam3d/releases/latest).

To install the latest unreleased code, download or clone [this source repository](https://github.com/WiseLabCMU/slam3d) and run the following to build and install:
```
python3 -m pip install .
```

### Example

```python3
import numpy as np
from particlefilter import ParticleFilterLoc, setSeed

# If deterministic tests are needed, call this before anything else
setSeed(123456789) # Argument is np.uint32

pf = ParticleFilterLoc()

pf.depositVio(t: np.float64, x: np.float32, y: np.float32, z: np.float32, dist: np.float32)
pf.depositRange(bx: np.float32, by: np.float32, bz: np.float32, range: np.float32, stdRange: np.float32)
pf.depositRssi(bx: np.float32, by: np.float32, bz: np.float32, rssi: np.int32)
pf.getTagLoc() # returns tuple: (status: np.int32, t: np.float64, x: np.float32, y: np.float32, z: np.float32, theta: np.float32)

```
### Development

To install for development, download or clone and run:
```
python3 -m pip install -e .
```
Source found in `./particlefilter/cython`.

## C

Source found in `./particlefilter`. A bundled version of MUSL `rand_r()` is included for Windows builds. Force it on other platforms with `-DPF_FORCE_MUSL_RANDR=1`. Precompiled shared libraries can be found in the [latest release](https://github.com/WiseLabCMU/slam3d/releases/latest).

### Shared library
```
gcc -fPIC -shared -o particlefilter.so -Iparticlefilter/include particlefilter/src/*.c -lm
```
Or on Windows with `cl.exe`:
```
cl.exe particlefilter/src/*.c /Iparticlefilter/include /MT /link /DLL /OUT:build/particlefilter.dll
```

### Development

Compile tests with:
```
gcc -o build/test -Iparticlefilter/include particlefilter/src/*.c test/test.c -lm
```

On Windows `cl.exe` also works (from Development Command Prompt):
```
cl.exe -o build/test -Iparticlefilter/include particlefilter/src/*.c test/test.c -lm
```

Compile and debug tests with:
```
gcc -o build/test -Iparticlefilter/include particlefilter/src/*.c test/test.c -lm -g
gdb ./build/test
```

## Tests

Both `test/test.c` and `test/test.py` implement mostly the same test.

```
test [--nofail] <test folder> <output file> [expected file (required without --nofail)]
```

The currently provided test folder is `./test/data`. `--nofail` will cause the test to always exit with status code 0 and allow omitting testing against an expected file (by not providing the last argument).

When running the C test, some expected files are provided in `./test/data` for various operating systems and compilers. When running the Python test, you should test against the output of running the C test on your system. The Python test will also for some floating point precision deviation. Since the Python test tests each value individually, it is impervious to CRLF/LF differences.

## License

Copyright (c) 2021, Wireless Sensing and Embedded Systems Lab, Carnegie Mellon University
All rights reserved.

This source code is licensed under the BSD-3-Clause license found in the
LICENSE file in the root directory of this source tree.
