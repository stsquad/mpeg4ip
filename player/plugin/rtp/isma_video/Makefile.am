libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = isma_enc_video_rtp_plugin.la
isma_enc_video_rtp_plugin_la_LDFLAGS = -module
isma_enc_video_rtp_plugin_la_SOURCES = \
	isma_enc_video_plugin.cpp isma_enc_video_plugin.h 

isma_enc_video_rtp_plugin_la_LIBADD = \
        @ISMACRYP_LIB@ \
	-lm 

INCLUDES = -I$(top_srcdir)/../mpeg4ip/player/src \
	   -I$(top_srcdir)/../mpeg4ip/player/lib \
	   -I$(top_srcdir)/../mpeg4ip/lib/SDL/include \
	   -I$(top_srcdir)/../mpeg4ip/include \
           @ISMACRYP_INC@ \
           -I$(top_srcdir)/../mpeg4ip/lib

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

