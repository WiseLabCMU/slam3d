#!/usr/bin/env python

from glob import glob
from Cython.Build import cythonize
import pathlib
from setuptools import setup, Extension

here = pathlib.Path(__file__).parent.resolve()

long_description = (here / 'README.md').read_text(encoding='utf-8')

extensions = [
    Extension("particleFilter",
              ["./particlefilter/cython/particleFilter.pyx"] + glob("./particlefilter/src/*.c"),
              include_dirs=["./particlefilter/include"]
    )
]

setup(
    name="slam3d",
    version="1.0.0",
    description="Simultaneous localization and mapping (SLAM) tools in 3D",
    long_description=long_description,
    long_description_content_type='text/markdown',
    ext_modules=cythonize(extensions),
    zip_safe=False,
    python_requires='>=3.6, <4',
    download_url="https://github.com/WiseLabCMU/slam3d",
    platforms=["any"],
    url="https://github.com/WiseLabCMU/slam3d",
    project_urls={
        'Bug Reports': 'https://github.com/WiseLabCMU/slam3d/issues',
        'Source': 'https://github.com/WiseLabCMU/slam3d',
    },
    # license='',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        # 'License :: OSI Approved :: ',
        'Programming Language :: C',
        'Programming Language :: Cython',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3 :: Only',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Information Analysis',
        'Topic :: Scientific/Engineering :: Mathematics'
    ],
    keywords='slam slam3d 3d particlefilter particle filter localize localization map mapping tool tools'
)
