libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = isma_rtp_plugin.la \
                  isma_enc_rtp_plugin.la

isma_rtp_plugin_la_LDFLAGS = -module
isma_enc_rtp_plugin_la_LDFLAGS = -module

isma_rtp_plugin_la_SOURCES = \
	isma_rtp_plugin.cpp isma_rtp_plugin.h

isma_enc_rtp_plugin_la_SOURCES = \
        isma_enc_rtp_plugin.cpp isma_enc_rtp_plugin.h

isma_rtp_plugin_la_LIBADD = \
	$(top_builddir)/lib/mp4av/libmp4av.la \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	-lm 

isma_enc_rtp_plugin_la_LIBADD = \
        $(top_builddir)/lib/mp4av/libmp4av.la \
        $(top_builddir)/player/lib/mp4util/libmp4util.la \
        $(top_builddir)/lib/ismacryp/libismacryp.la @SRTPLIB@ \
        -lm

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib/ismacryp \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/sdp \
           -I$(top_srcdir)/lib/rtp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = isma_audio_rtp_plugin.dsp ismacryp_rtp_audio.dsp \
	isma_audio_rtp_plugin.vcproj ismacryp_rtp_audio.vcproj
