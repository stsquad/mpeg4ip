AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

noinst_LTLIBRARIES = libobj_type.la
libobj_type_la_LDFLAGS = -static

libobj_type_la_SOURCES = \
			basic.cpp \
			geom.cpp \
			grayc.cpp \
			grayf.cpp \
			grayi.cpp \
			svd.cpp \
			typeapi.cpp \
			vop.cpp \
			warp.cpp \
			yuvac.cpp \
			yuvai.cpp
