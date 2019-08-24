# distutils: language=c++

import youtube_dl
from libcpp.string cimport string
from libcpp.map cimport map
from .stdany cimport any

cdef public api class YoutubeDL [object YoutubeDL, type YoutubeDLType]:
	ytdl = None

	def __init__(self, params):
		self.ytdl = youtube_dl.YoutubeDL(params)

	cdef public api getInfo(self, video: string):
		return self.ytdl.extract_info(video, download=False)
