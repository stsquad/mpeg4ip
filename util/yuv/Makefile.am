bin_PROGRAMS = yuvdump

check_PROGRAMS = yuvcreate yuvdisp

yuvdump_SOURCES = yuvdump.cpp

yuvcreate_SOURCES = yuvcreate.cpp

yuvdisp_SOURCES = yuvdisp.cpp

INCLUDES = -I. -I../../lib/SDL/include

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

LDADD = \
	@SDL_LIBS@ \
	$(top_builddir)/lib/SDL/src/main/libSDLmain.a \
        -lm

