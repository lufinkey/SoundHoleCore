# distutils: language=c++

import youtube_dl
from libcpp.string cimport string
from libcpp.map cimport map
from .stdany cimport any

cdef public class YoutubeDL [object YoutubeDL, type YoutubeDLType]:
	api = None

	def __init__(self, params):
		self.api = youtube_dl.YoutubeDL(params)

cdef public YoutubeDL YoutubeDL_create(params):
	return YoutubeDL(params)

cdef public YoutubeDL YoutubeDL_getInfo(ytdl: YoutubeDL, video: string):
	return ytdl.api.extract_info(video, download=False)
