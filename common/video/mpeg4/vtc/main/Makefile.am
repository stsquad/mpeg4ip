AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type
noinst_LTLIBRARIES = libobj_vtc_main.la libobj_vtc_main_dec.la libobj_vtc_main_enc.la

libobj_vtc_main_la_LDFLAGS = -static

libobj_vtc_main_la_SOURCES = \
			computePSNR.cpp \
			read_image.cpp \
			seg.cpp \
			wavelet.cpp \
			write_image.cpp

libobj_vtc_main_dec_la_LDFLAGS = -static
libobj_vtc_main_dec_la_SOURCES = \
			vtcdec.cpp

libobj_vtc_main_enc_la_LDFLAGS = -static
libobj_vtc_main_enc_la_SOURCES = \
			vtcenc.cpp \
