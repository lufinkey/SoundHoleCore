# distutils: language=c++

import youtube_dl
from libcpp.string cimport string
from libcpp.map cimport map
from .stdany cimport any

cdef map[string,any] Map

cdef public class YoutubeDL [object YoutubeDL, type YoutubeDLType]:
	ytdl = None

	def __init__(self, params: Map):
		ytdl_params = {}
		ytdl = youtube_dl.YoutubeDL(ytdl_params)
