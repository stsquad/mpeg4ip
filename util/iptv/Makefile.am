bin_PROGRAMS = iptv_prog

iptv_prog_SOURCES = iptv_prog.cpp

INCLUDES = -I. -I../../include -I../../lib/SDL/include \
	-I$(top_builddir)/player/lib/libhttp \
	-I$(top_builddir)/lib/sdp

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

iptv_prog_LDADD = \
	@SDL_LIBS@ \
	$(top_builddir)/lib/SDL/src/main/libSDLmain.a \
	$(top_builddir)/player/lib/libhttp/libhttp.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
        -lm
