
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4v2

AM_CXXFLAGS = -DMPEG4IP -fexceptions -Wall

if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif

bin_PROGRAMS = mp4creator

mp4creator_SOURCES = \
	mp4creator.h \
	mp4creator.cpp \
	aac.cpp \
	mp3.cpp \
	mp4v.cpp \
	rfc2250.cpp \
	rfcisma.cpp \
	rfc3016.cpp

mp4creator_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la $(LDADD_E1)

