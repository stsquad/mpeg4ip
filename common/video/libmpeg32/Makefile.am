SUBDIRS = video

noinst_LTLIBRARIES = libmpeg3.la libmpeg3util.la

INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4av -I$(top_srcdir)/lib/mp4v2 
AM_CFLAGS= -DHAVE_CSS
libmpeg3util_la_SOURCES = \
	bitstream.c \
	bitstream.h 

libmpeg3_la_SOURCES = \
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
	workarounds.h \
	mpeg3privateinc.h

EXTRA_DIST = mpeg3css_fake.c libmpeg32.dsp  \
	libmpeg32util.dsp libmpeg32_st.dsp \
	libmpeg32.xcode/project.pbxproj


