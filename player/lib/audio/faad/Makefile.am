lib_LTLIBRARIES = libfaad.la
libfaad_la_SOURCES = \
	wav.c \
	filestream.c \
	block.c \
	config.c \
	decdata.c \
	decoder.c \
	dolby_adapt.c \
	fastfft.c \
	huffdec1.c \
	huffdec2.c \
	huffdec3.c \
	huffinit.c \
	hufftables.c \
	intensity.c \
	nok_lt_prediction.c \
	portio.c \
	stereo.c \
	tns.c \
	transfo.c \
	pns.c

AM_CFLAGS = -D_REENTRANT -DHAS_ULONG -Werror -Wall -fexceptions -fno-rtti 

check_PROGRAMS = faad
faad_SOURCES = faad.c
LDADD = ./libfaad.la -lm

