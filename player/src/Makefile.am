###########################################################################
# Some rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

SUBDIRS = codec

noinst_LTLIBRARIES = libmp4playerutils.la libmp4player.la
libmp4player_la_LDFLAGS = -static

if USE_MMX
EXTRA_libmp4player_la_SOURCES = frame_doubler_mmx.asm
libmp4player_la_LIBADD = frame_doubler_mmx.lo
else
EXTRA_libmp4player_la_SOURCES = frame_doubler.cpp
libmp4player_la_LIBADD = frame_doubler.lo
endif

libmp4playerutils_la_SOURCES = \
	our_bytestream_file.cpp \
	player_util.c 

libmp4player_la_SOURCES = \
	audio.cpp \
	avi_bytestream.cpp \
	avi_file.cpp \
	codec_plugin.cpp \
	ip_port.cpp \
	media_utils.cpp \
	our_config_file.cpp \
	mp4_file.cpp \
	mp4_bytestream.cpp \
	player_sdp.c \
	player_media.cpp \
	player_media_decode.cpp \
	player_session.cpp \
	playlist.cpp \
	qtime_bytestream.cpp \
	qtime_file.cpp \
	rtp_bytestream.cpp \
	sync.cpp \
	video.cpp 

if NO_GLIB_GTK
PROG_E1 =
else
PROG_E1 = gmp4player
endif

bin_PROGRAMS = mp4player $(PROG_E1)

check_PROGRAMS = movplayer

mp4player_SOURCES = \
	main.cpp 

gmp4player_SOURCES = \
	gui_main.cpp \
	gui_showmsg.c \
	gui_utils.c 

movplayer_SOURCES = player_util.c \
	movplayer.cpp

INCLUDES = -I. -I$(top_srcdir)/common/video/mpeg4 -I./codec -I./codec/mpeg4 -I../lib -I../lib/audio -I$(top_srcdir)/lib/SDL/include -I$(top_srcdir)/lib/mp4 -I$(top_srcdir)/lib/mp4v2 -I$(top_srcdir)/include -I$(top_srcdir)/include -I$(top_srcdir)/lib/avi -I$(top_srcdir)/lib @GLIB_CFLAGS@ @GTK_CFLAGS@ @SDL_CFLAGS@ @X_CFLAGS@

AM_CFLAGS= -D_REENTRANT @BILLS_CWARNINGS@ -DPLAYER_PLUGIN_DIR=\"@PLAYER_PLUGIN_DIR@\"

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -DPLAYER_PLUGIN_DIR=\"@PLAYER_PLUGIN_DIR@\"

mp4player_LDADD = \
	-lm \
	$(top_builddir)/player/src/libmp4player.la \
	$(top_builddir)/player/src/codec/aa/libaa.la \
	$(top_builddir)/player/src/codec/mp3/libmp3.la \
	$(top_builddir)/lib/sdp/libsdp.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	$(top_builddir)/player/lib/libhttp/libhttp.la \
	$(top_builddir)/lib/config_file/libconfig_file.la \
	$(top_builddir)/lib/msg_queue/libmsg_queue.la \
	$(top_builddir)/lib/bitstream/libbitstream.la \
	$(top_builddir)/lib/rtp/libuclmmbase.la \
	$(top_builddir)/player/lib/rtsp/librtsp.la \
	$(top_builddir)/lib/mp4/libmp4.la \
	$(top_builddir)/lib/mp4v2/libmp4v2.la \
	$(top_builddir)/lib/avi/libavi.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	libmp4playerutils.la \
	@SDL_LIBS@ \
	$(top_builddir)/lib/SDL/src/main/libSDLmain.a

gmp4player_LDADD = $(mp4player_LDADD) \
	@GTK_LIBS@ @GLIB_LIBS@


