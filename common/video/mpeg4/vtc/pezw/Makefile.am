AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type
noinst_LTLIBRARIES = libobj_vtc_pezw.la
libobj_vtc_pezw_la_LDFLAGS = -static

libobj_vtc_pezw_la_SOURCES = \
			PEZW_ac.cpp \
			PEZW_utils.cpp \
			PEZW_textureLayerBQ.cpp \
			wvtpezw_tree_init_encode.cpp \
			wvtpezw_tree_encode.cpp \
			wvtPEZW.cpp \
			wvtpezw_tree_init_decode.cpp \
			wvtpezw_tree_decode.cpp \
			PEZW_globals.cpp
