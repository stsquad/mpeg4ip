noinst_LTLIBRARIES = libdivx.la
libdivx_la_LDFLAGS = -static
libdivx_la_SOURCES = \
	divx.cpp \
	divx_file.cpp

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I../../../lib/video/divx -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

