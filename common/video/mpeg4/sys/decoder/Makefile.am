AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include

noinst_LTLIBRARIES = libobj_sys_decoder.la
libobj_sys_decoder_la_LDFLAGS = -static

libobj_sys_decoder_la_SOURCES = \
			blkdec.cpp \
			errdec.cpp \
			mbdec.cpp \
			mbheaddec.cpp \
			mbinterlacedec.cpp \
			mcdec.cpp \
			mvdec.cpp \
			rvlcdec.cpp \
			shpdec.cpp \
			sptdec.cpp \
			vopmbdec.cpp \
			vopsedec.cpp
