noinst_LTLIBRARIES = libdivx.la
libdivx_la_LDFLAGS = -static
libdivx_la_SOURCES = \
	divx.cpp \
	divx_file.cpp

INCLUDES = -I.. -I../.. -I../../../lib -I../../../lib/SDL/include -I../../../lib/video/divx -I../../../../common/video/mpeg4 -I../../../../include

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

