noinst_LTLIBRARIES = libmpeg4.la
libmpeg4_la_LDFLAGS = -static
libmpeg4_la_SOURCES = \
	mpeg4.cpp \
	mpeg4_file.cpp

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

