lib_LTLIBRARIES = libffmpeg.la
libffmpeg_la_LDFLAGS = -static

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -DMPEG4IP -O2

libffmpeg_la_SOURCES = \
	avcodec.h \
	avcodec.c \
	common.c \
	common.h \
	dsputil.c \
	dsputil.h \
	fdctref.c \
	h263data.h \
	h263.c \
	h263dec.c \
	imgconvert.c \
	imgresample.c \
	jfdctfst.c \
	jrevdct.c \
	motion_est.c \
	mjpeg.c \
	mpeg12.c \
	mpeg12data.h \
	mpeg4data.h \
	mpegencodevlc.h \
	mpegvideo.c \
	mpegvideo.h \
	resample.c

