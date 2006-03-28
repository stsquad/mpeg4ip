lib_LTLIBRARIES = libsrtpif.la

INCLUDES=-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

libsrtpif_la_SOURCES = liblibsrtp.c \
	liblibsrtp.h

#EXTRA_DIST = README README.html
