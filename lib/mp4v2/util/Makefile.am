
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4v2

AM_CXXFLAGS = -Wall

bin_PROGRAMS = mp4dump mp4extract mp4info mp4trackdump mp4tags

mp4dump_SOURCES = mp4dump.cpp
mp4dump_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4extract_SOURCES = mp4extract.cpp
mp4extract_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4info_SOURCES = mp4info.cpp
mp4info_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4trackdump_SOURCES = mp4trackdump.cpp
mp4trackdump_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

mp4tags_SOURCES = mp4trackdump.cpp
mp4tags_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

EXTRA_DIST = mp4dump60.dsp mp4extract60.dsp \
	mp4info.dsp
