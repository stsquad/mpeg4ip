lib_LTLIBRARIES = libsdp.la
libsdp_la_LDFLAGS = -static
libsdp_la_SOURCES = \
	sdp_decode.c \
	sdp_encode.c \
	sdp_dump.c \
	sdp_util.c

INCLUDES = -I../../../include

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

check_PROGRAMS = test
test_SOURCES = test.c
test_LDADD = libsdp.la

