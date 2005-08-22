check_PROGRAMS = iptv_prog

iptv_prog_SOURCES = iptv_prog.cpp

INCLUDES = -I$(srcdir) -I$(top_srcdir)/include \
	-I$(top_srcdir)/player/lib/libhttp \
	-I$(top_srcdir)/lib/sdp

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

iptv_prog_LDADD = \
	@SDL_LIBS@ \
	-lSDLmain \
	$(top_builddir)/player/lib/libhttp/libhttp.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
        -lm
