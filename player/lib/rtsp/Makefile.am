lib_LTLIBRARIES = librtsp.la
librtsp_la_LDFLAGS = -static
librtsp_la_SOURCES = \
	rtsp_comm.c \
	rtsp_command.c \
	rtsp_resp.c \
	rtsp_util.c 

INCLUDES = -I../../../lib/sdp -I../../../include
AM_CFLAGS = -DIPTV_COMPATIBLE -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

check_PROGRAMS = test
test_SOURCES = test.c
test_INCLUDES = -I../../../lib/sdp
test_LDADD = librtsp.la ../../../lib/sdp/libsdp.la

