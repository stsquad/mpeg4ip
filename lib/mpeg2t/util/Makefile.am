
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/lib/rtp -I$(top_srcdir)/lib/mpeg2t -I$(top_srcdir)/lib/SDL/include

AM_CFLAGS = @BILLS_CWARNINGS@
AM_CXXFLAGS = @BILLS_CPPWARNINGS@

bin_PROGRAMS = mpeg2t_record

mpeg2t_record_SOURCES = mpeg2t_record.cpp
mpeg2t_record_LDADD = \
	$(top_builddir)/lib/rtp/libuclmmbase.la \
	$(top_builddir)/lib/mpeg2t/libmpeg2_transport.la \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	@SDL_LIBS@ \
	$(top_builddir)/lib/mp4v2/libmp4v2.la 



