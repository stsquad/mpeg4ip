
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/avi

AM_CXXFLAGS = -DMPEG4IP -fexceptions -Wall

bin_PROGRAMS = mp4creator

mp4creator_SOURCES = \
	mp4creator.h \
	mp4creator.cpp \
	aac.cpp \
	mp3.cpp \
	mp4v.cpp \
	avi.cpp

mp4creator_LDADD = \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/avi/libavi.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

EXTRA_DIST = mp4creator60.dsp mp4creator60.dsw
