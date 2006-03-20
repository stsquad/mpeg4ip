lib_LTLIBRARIES = libsrtpif.la

INCLUDES=-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/rtp

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

libsrtpif_la_SOURCES = liblibsrtp.c \
	liblibsrtp.h

#EXTRA_DIST = README README.html
