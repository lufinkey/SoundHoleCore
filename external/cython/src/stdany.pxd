# distutils: language=c++

from libcpp cimport bool

cdef extern from "<any>" namespace "std" nogil:
	cdef cppclass any:
		any()
		any(const any&)
		
		void reset()
		bool has_value()

	cdef T any_cast[T](...) except +
