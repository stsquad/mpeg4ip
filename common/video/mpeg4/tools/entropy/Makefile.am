AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = \
	-I../../../../../include \
	-I../../type

noinst_LTLIBRARIES = libobj_tools_entropy.la
libobj_tools_entropy_la_LDFLAGS = -static

libobj_tools_entropy_la_SOURCES = \
	bitstrm.cpp \
	huffman.cpp \
