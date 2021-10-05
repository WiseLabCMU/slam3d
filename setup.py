#!/usr/bin/env python
"""
setup.py
Created by Perry Naseck on 6/20/21.

Copyright (c) 2021, Wireless Sensing and Embedded Systems Lab, Carnegie
Mellon University
All rights reserved.

This source code is licensed under the BSD-3-Clause license found in the
LICENSE file in the root directory of this source tree.
"""

from glob import glob
from Cython.Build import cythonize
import pathlib
from setuptools import setup, Extension
import numpy

here = pathlib.Path(__file__).parent.resolve()

long_description = (here / 'README.md').read_text(encoding='utf-8')

extensions = [
    Extension("particlefilter",
              ["./particlefilter/cython/particlefilter_wrapper.pyx"] + glob("./particlefilter/src/*.c"),
              include_dirs=["./particlefilter/include", numpy.get_include()]
    )
]

setup(
    name="slam3d",
    description="Simultaneous localization and mapping (SLAM) tools in 3D",
    long_description=long_description,
    long_description_content_type='text/markdown',
    license='BSD-3-Clause',
    author='Wireless Sensing and Embedded Systems Lab, Carnegie Mellon University',
    author_email='wiselabCMU@gmail.com',
    ext_modules=cythonize(extensions),
    zip_safe=False,
    python_requires='>=3.6, <4',
    platforms=["any"],
    url="https://github.com/WiseLabCMU/slam3d",
    project_urls={
        'Bug Reports': 'https://github.com/WiseLabCMU/slam3d/issues',
        'Source': 'https://github.com/WiseLabCMU/slam3d',
    },
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: C',
        'Programming Language :: Cython',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3 :: Only',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Information Analysis',
        'Topic :: Scientific/Engineering :: Mathematics'
    ],
    keywords='slam slam3d 3d particlefilter particle filter localize localization map mapping tool tools',
    install_requires=[
        'numpy>=1,<2'
    ]
)
