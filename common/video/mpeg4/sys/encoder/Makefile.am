AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include

noinst_LTLIBRARIES = libobj_sys_encoder.la
libobj_sys_encoder_la_LDFLAGS = -static

libobj_sys_encoder_la_SOURCES = \
			paramset.cpp \
			blkenc.cpp \
			errenc.cpp \
			mbenc.cpp \
			mbinterlace.cpp \
			mcenc.cpp \
			motest.cpp \
			mvenc.cpp \
			padenc.cpp \
			sesenc.cpp \
			shpenc.cpp \
			sptenc.cpp \
			tm5rc.cpp \
			vopmbenc.cpp \
			vopseenc.cpp
