
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/mp4v2 \
	-I$(top_srcdir)/lib/mp4av \
	-I$(top_srcdir)/lib/avi \
	@ISMACRYP_INC@ \
	-I$(top_srcdir)/common/video/libmpeg32

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
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/lib/avi/libavi.la \
	@ISMACRYP_LIB@ \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/common/video/libmpeg32/libmpeg3.la

EXTRA_DIST = mp4creator60.dsp mp4creator.xcode/project.pbxproj
