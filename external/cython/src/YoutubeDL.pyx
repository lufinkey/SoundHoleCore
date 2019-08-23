# distutils: language=c++

import youtube_dl
from libcpp.string cimport string
from libcpp.map cimport map
from libcpp cimport bool

cdef extern from "<any>" namespace "std" nogil:
	cdef cppclass any:
		any()
		any(const any&)
		
		void reset()
		bool has_value()

	cdef T any_cast[T](...) except +

cdef map[string,any] Map

cdef public class YoutubeDL [object YoutubeDL, type YoutubeDLType]:
	ytdl = None

	def __init__(self, params: Map):
		ytdl_params = {}
		ytdl = youtube_dl.YoutubeDL(ytdl_params)
