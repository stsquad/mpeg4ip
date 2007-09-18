bin_PROGRAMS = yuvdump sdl_pcm_play

check_PROGRAMS = yuvcreate yuvdisp

yuvdump_SOURCES = yuvdump.cpp

yuvcreate_SOURCES = yuvcreate.cpp

yuvdisp_SOURCES = yuvdisp.cpp

sdl_pcm_play_SOURCES = sdlsoundplay.cpp 

INCLUDES = -I$(srcdir) -I$(top_srcdir)/include

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

LDADD = \
	@SDL_LIBS@ \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
        -lm

EXTRA_DIST=yuvdump60.dsp yuvdump.vcproj
