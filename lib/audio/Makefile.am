noinst_LTLIBRARIES = libaudio.la
libaudio_la_SOURCES = \
	audio_convert.cpp \
	audio_convert.h

INCLUDES = -I$(top_builddir)/include 
AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT @BILLS_CPPWARNINGS@

EXTRA_DIST=audio.dsp
