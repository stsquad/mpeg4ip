
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/avi \
	-I$(top_srcdir)/lib/ismacryp \
	-I$(top_srcdir)/lib/mpeg2ps

AM_CXXFLAGS = -DMPEG4IP -fexceptions @BILLS_CPPWARNINGS@

bin_PROGRAMS = mp4creator

mp4creator_SOURCES = \
	aac.cpp \
	amr.cpp \
	avi.cpp \
	h263.cpp \
	h264.cpp \
	mp3.cpp \
	mp4creator.h \
	mp4creator.cpp \
	mp4v.cpp \
	mpeg.cpp \
	mpeg.h 

mp4creator_LDADD = \
	$(top_builddir)/lib/mpeg2ps/libmpeg2_program.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/avi/libavi.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/lib/ismacryp/libismacryp.la \
	@SRTPLIB@ 

EXTRA_DIST = mp4creator60.dsp mp4creator.vcproj
