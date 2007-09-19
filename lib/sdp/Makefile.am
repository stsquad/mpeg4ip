lib_LTLIBRARIES = libsdp.la
include_HEADERS = \
	sdp.h \
	sdp_error.h 

libsdp_la_SOURCES = \
	sdp_decode.c \
	sdp_encode.c \
	sdp_dump.c \
	sdp_util.c \
	sdp_decode_private.h

EXTRA_DIST = libsdp60.dsp libsdp.vcproj

INCLUDES = -I$(top_srcdir)/include 

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

check_PROGRAMS = test_sdp
test_sdp_SOURCES = test.c
test_sdp_LDADD = libsdp.la 

