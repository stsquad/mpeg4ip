bin_PROGRAMS = yuvdump

check_PROGRAMS = yuvcreate yuvdisp

yuvdump_SOURCES = yuvdump.cpp

yuvcreate_SOURCES = yuvcreate.cpp

yuvdisp_SOURCES = yuvdisp.cpp

INCLUDES = -I. -I../../player/lib/SDL/include

AM_CFLAGS= -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -fno-rtti -Wall -Werror

LDADD = \
        $(top_builddir)/player/lib/SDL/src/libSDL.la \
        -lpthread \
        -lm

