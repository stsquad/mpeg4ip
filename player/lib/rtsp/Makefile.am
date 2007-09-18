noinst_LTLIBRARIES = librtsp.la
librtsp_la_SOURCES = \
	rtsp.c \
	rtsp_client.h \
	rtsp_comm.c \
	rtsp_command.c \
	rtsp_private.h \
	rtsp_resp.c \
	rtsp_thread.c \
	rtsp_thread_ipc.h \
	rtsp_thread_nx.c \
	rtsp_thread_nx.h \
	rtsp_util.c 

INCLUDES = -I$(top_srcdir)/lib/sdp -I$(top_srcdir)/include -I$(top_srcdir)/lib 
AM_CFLAGS = -DIPTV_COMPATIBLE -D_REENTRANT @BILLS_CWARNINGS@ 

check_PROGRAMS = test test_thread
test_SOURCES = test.c
test_INCLUDES = -I$(top_srcdir)/lib/sdp
test_LDADD = librtsp.la $(top_builddir)/lib/sdp/libsdp.la @SDL_LIBS@ \
	$(top_builddir)/lib/rtp/libuclmmbase.la $(top_builddir)/lib/utils/libutils.la
test_thread_SOURCES = test_thread.c
test_thread_INCLUDES = -I../../../lib/sdp
test_thread_LDADD = librtsp.la $(top_builddir)/lib/sdp/libsdp.la @SDL_LIBS@ \
	$(top_builddir)/lib/rtp/libuclmmbase.la $(top_builddir)/lib/utils/libutils.la

EXTRA_DIST = librtsp60.dsp rtsp_thread_win.cpp rtsp_thread_win.h librtsp.vcproj
