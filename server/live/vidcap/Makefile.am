noinst_LTLIBRARIES = libvidcap.la

INCLUDES=-I$(top_srcdir)/include

AM_CFLAGS= -Wall -O2 -g -D_FILE_OFFSET_BITS=64

libvidcap_la_SOURCES = \
	byteswap.h \
	capture.c \
	capture.h \
	color_common.c \
	color_packed.c \
	color_yuv2rgb.c \
	grab-bsd.c \
	grab-ng.c \
	grab-ng.h \
	grab-v4l.c \
	grab-v4l2.c \
	videodev2.h \
	writeavi.c \
	writefile.c \
	writefile.h \
	writeqt.c

