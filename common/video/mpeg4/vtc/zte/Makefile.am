AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type
noinst_LTLIBRARIES = libobj_vtc_zte.la
libobj_vtc_zte_la_LDFLAGS = -static

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
