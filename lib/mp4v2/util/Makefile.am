
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4v2

AM_CXXFLAGS = -Wall

if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif

bin_PROGRAMS = mp4broadcaster mp4dump mp4extract

mp4broadcaster_SOURCES = mp4broadcaster.cpp
mp4broadcaster_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la $(LDADD_E1)

mp4dump_SOURCES = mp4dump.cpp
mp4dump_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la $(LDADD_E1)

mp4extract_SOURCES = mp4extract.cpp
mp4extract_LDADD = $(top_builddir)/lib/mp4v2/libmp4v2.la $(LDADD_E1)

