noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = \
	message.c \
	network.c \
	url.c \
	mpeg4ip_utils.h

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@
AM_CXXFLAGS = -D_REENTRANT -fexceptions @BILLS_CPPWARNINGS@ 


