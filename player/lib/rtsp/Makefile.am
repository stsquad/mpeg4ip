noinst_LTLIBRARIES = librtsp.la
librtsp_la_LDFLAGS = -static
librtsp_la_SOURCES = \
	rtsp.c \
	rtsp_comm.c \
	rtsp_command.c \
	rtsp_resp.c \
	rtsp_thread_nx.c \
	rtsp_thread.c \
	rtsp_util.c 

INCLUDES = -I$(top_builddir)/lib/sdp -I$(top_builddir)/include -I$(top_builddir)/lib/SDL/include -I$(top_builddir)/lib
AM_CFLAGS = -DIPTV_COMPATIBLE -D_REENTRANT @BILLS_CWARNINGS@ @SDL_CFLAGS@

check_PROGRAMS = test test_thread
test_SOURCES = test.c
test_INCLUDES = -I../../../lib/sdp
test_LDADD = librtsp.la ../../../lib/sdp/libsdp.la @SDL_LIBS@ \
	../../../lib/rtp/libuclmmbase.la
test_thread_SOURCES = test_thread.c
test_thread_INCLUDES = -I../../../lib/sdp
test_thread_LDADD = librtsp.la ../../../lib/sdp/libsdp.la @SDL_LIBS@ \
	../../../lib/rtp/libuclmmbase.la
