AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type -I../shape
noinst_LTLIBRARIES = libobj_vtc_zte.la

libobj_vtc_zte_la_SOURCES = \
	ac.cpp \
	ac.hpp \
	bitpack.cpp \
	bitpack.hpp \
	startcode.hpp \
	ztscan_common.hpp \
	ztscan_dec.cpp \
	ztscan_enc.cpp \
	ztscanUtil.cpp \
	ztscanUtil.hpp
