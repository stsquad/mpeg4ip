AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type
noinst_LTLIBRARIES = libobj_vtc_zte.la
libobj_vtc_zte_la_LDFLAGS = -static

libobj_vtc_zte_la_SOURCES = \
			ac.cpp \
			bitpack.cpp \
			ztscanUtil.cpp \
			ztscan_dec.cpp \
			ztscan_enc.cpp \
