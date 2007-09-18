libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = isma_enc_video_rtp_plugin.la
isma_enc_video_rtp_plugin_la_LDFLAGS = -module
isma_enc_video_rtp_plugin_la_SOURCES = \
	isma_enc_video_plugin.cpp isma_enc_video_plugin.h 

isma_enc_video_rtp_plugin_la_LIBADD = \
        $(top_builddir)/lib/ismacryp/libismacryp.la @SRTPLIB@ \
	$(top_builddir)/player/lib/mp4util/libmp4util.la \
	-lm 

INCLUDES = -I$(top_srcdir)/player/src \
	   -I$(top_srcdir)/player/lib \
	   -I$(top_srcdir)/lib/SDL/include \
	   -I$(top_srcdir)/include \
           -I$(top_srcdir)/lib/ismacryp \
           -I$(top_srcdir)/lib \
           -I$(top_srcdir)/lib/rtp \
           -I$(top_srcdir)/lib/sdp

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = ismacryp_rtp_video_plugin.dsp ismacryp_rtp_video_plugin.vcproj
