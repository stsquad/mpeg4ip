AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type -I../shape

noinst_LTLIBRARIES = libobj_vtc_pezw.la

libobj_vtc_pezw_la_SOURCES = \
	PEZW_ac.cpp \
	PEZW_ac.hpp \
	PEZW_functions.hpp \
	PEZW_globals.cpp \
	PEZW_mpeg4.hpp \
	PEZW_textureLayerBQ.cpp \
	PEZW_utils.cpp \
	PEZW_zerotree.hpp \
	wvtPEZW.cpp \
	wvtPEZW.hpp \
	wvtpezw_tree_codec.hpp \
	wvtpezw_tree_decode.cpp \
	wvtpezw_tree_encode.cpp \
	wvtpezw_tree_init_decode.cpp \
	wvtpezw_tree_init_encode.cpp
