libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = h261_plugin.la
h261_plugin_la_LDFLAGS = -module
h261_plugin_la_SOURCES = \
	bsd-endian.h \
	bv.c \
	dct.cpp \
	dct.h \
	huffcode.c \
	ourh261.cpp \
	ourh261.h \
	p64.cpp \
	p64dump.cpp \
	p64dump.h \
	p64.h \
	p64-huff.h

h261_plugin_la_LIBADD = \
	-lm 

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/player/src -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/player/plugin/rtp/h261 -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@ -DINT64=uint64_t

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -DINT64=uint64_t

EXTRA_DIST= README.vic
