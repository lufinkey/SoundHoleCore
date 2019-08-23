
from distutils.core import setup
from Cython.Build import cythonize
import os

os.environ['CFLAGS'] = '-O3 -Wall -std=c++17'
setup(
	ext_modules = cythonize(
		"src/*.pyx"
	)
)
