lib_LTLIBRARIES = libsrtpif.la

INCLUDES=-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib/rtp \
	-I$(top_srcdir)/lib/utils \
	-I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

libsrtpif_la_SOURCES = liblibsrtp.c \
	liblibsrtp.h

#EXTRA_DIST = README README.html

check_PROGRAMS = test_srtp_client test_srtp_server

test_srtp_client_SOURCES = test_srtp_client.cpp
test_srtp_client_LDADD = ../rtp/libuclmmbase.la libsrtpif.la ../mp4v2/libmp4v2.la ../../../srtp/libsrtp.a

test_srtp_server_SOURCES = test_srtp_server.cpp
test_srtp_server_LDADD = ../rtp/libuclmmbase.la libsrtpif.la ../mp4v2/libmp4v2.la ../../../srtp/libsrtp.a

EXTRA_DIST=liblibstrp.vcproj
