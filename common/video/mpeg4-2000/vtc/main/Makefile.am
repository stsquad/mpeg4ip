AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type -I../shape
noinst_LTLIBRARIES = libobj_vtc_main.la libobj_vtc_main_dec.la libobj_vtc_main_enc.la

libobj_vtc_main_la_SOURCES = \
			computePSNR.cpp \
			read_image.cpp \
			seg.cpp \
			wavelet.cpp \
			write_image.cpp

libobj_vtc_main_dec_la_SOURCES = \
			vtcdec.cpp

libobj_vtc_main_enc_la_SOURCES = \
			vtcenc.cpp
