lib_LTLIBRARIES = libfaad.la
libfaad_la_LDFLAGS = -static
libfaad_la_SOURCES = \
	block.c \
	config.c \
	decdata.c \
	decoder.c \
	dolby_adapt.c \
	fastfft.c \
	huffdec.c \
	huffinit.c \
	hufftables.c \
	intensity.c \
	monopred.c \
	mpeg4ip_bits.c \
	nok_lt_prediction.c \
	pns.c \
	stereo.c \
	tns.c \
	transfo.c \
	util.c

libfaad_la_INCLUDES = \
	all.h \
	bits.h \
	block.h \
	fastfft.h \
	interface.h \
	kbd_win.h \
	monopred.h \
	nok_lt_prediction.h \
	nok_ltp_common.h \
	nok_ltp_common_internal.h \
	port.h \
	tns.h \
	transfo.h\
	util.h

AM_CFLAGS = -O2 -DHAS_ULONG -Werror -Wall -fexceptions 


