SUBDIRS = audio video

noinst_LTLIBRARIES = libmpeg3.la

INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS= -DHAVE_CSS
libmpeg3_la_LDFLAGS = -static 
libmpeg3_la_SOURCES = \
	bitstream.c \
	bitstream.h \
	ifo.h \
	libmpeg3.c \
	libmpeg3.h \
	mpeg3atrack.c \
	mpeg3atrack.h \
	mpeg3css.c \
	mpeg3css.h \
	mpeg3demux.c \
	mpeg3demux.h \
	mpeg3ifo.c \
	mpeg3io.c \
	mpeg3io.h \
	mpeg3private.h \
	mpeg3protos.h \
	mpeg3title.c \
	mpeg3title.h \
	mpeg3vtrack.c \
	mpeg3vtrack.h \
	timecode.h \
	workarounds.c \
	workarounds.h

libmpeg3_la_LIBADD = \
	audio/libmpeg3audio.la \
	video/libmpeg3video.la 

