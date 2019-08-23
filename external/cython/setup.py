
from distutils.core import setup
from Cython.Build import cythonize
import os

os.environ['CFLAGS'] = '-O3 -Wall -std=c++17'
setup(
	name = "YoutubeDL",
	ext_modules = cythonize(
		"lib/*.pyx"
	)
)
