bin_PROGRAMS = yuvdump

check_PROGRAMS = yuvcreate yuvdisp

yuvdump_SOURCES = yuvdump.cpp

yuvcreate_SOURCES = yuvcreate.cpp

yuvdisp_SOURCES = yuvdisp.cpp

INCLUDES = -I. -I../../lib/SDL/include

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

LDADD = \
        $(top_builddir)/lib/SDL/src/libSDL.la \
        -lpthread \
        -lm

