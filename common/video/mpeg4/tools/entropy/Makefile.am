AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

INCLUDES = \
	-I../../../../../include \
	-I../../type

noinst_LTLIBRARIES = libobj_tools_entropy.la
libobj_tools_entropy_la_LDFLAGS = -static

libobj_tools_entropy_la_SOURCES = \
	bitstrm.cpp \
	bytestrm.cpp \
	huffman.cpp
