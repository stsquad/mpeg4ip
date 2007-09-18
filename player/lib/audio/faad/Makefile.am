lib_LTLIBRARIES = libmpeg4ip_faad.la
libmpeg4ip_faad_la_SOURCES = \
	bits.c \
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
	nok_lt_prediction.c \
	pns.c \
	stereo.c \
	tns.c \
	transfo.c \
	util.c \
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
INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = -O2 -Werror -Wall -fexceptions -fno-strict-aliasing

EXTRA_DIST = COPYING libfaad60.dsp README libfaad.vcproj
