lib_LTLIBRARIES = libffmpeg.la
libffmpeg_la_LDFLAGS = -static

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -DMPEG4IP -O2

libffmpeg_la_SOURCES = \
	avcodec.h \
	common.c \
	common.h \
	dsputil.c \
	dsputil.h \
	h263data.h \
	h263enc.c \
	jfdctfst.c \
	jrevdct.c \
	motion_est.c \
	mjpegenc.c \
	mpeg4data.h \
	mpegencodevlc.h \
	mpegvideo.c \
	mpegvideo.h 

