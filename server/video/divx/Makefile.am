bin_PROGRAMS = divxenc
divxenc_SOURCES = divxenc.c

INCLUDES = -I$(top_srcdir)/include
LDADD = $(top_builddir)/server/video/divx/libdivxencore.la -lm


lib_LTLIBRARIES = libdivxencore.la
libdivxencore_la_SOURCES = \
	bitstream.c \
	bitstream.h \
	encore.c \
	encore.h \
	max_level.h \
	mom_access.c \
	mom_access.h \
	mom_structs.h \
	mom_util.c \
	mom_util.h \
	momusys.h \
	mot_code.c \
	mot_code.h \
	mot_est_comp.c \
	mot_est_comp.h \
	mot_est_mb.c \
	mot_est_mb.h \
	mot_util.c \
	mot_util.h \
	non_unix.h \
	putvlc.c \
	putvlc.h \
	rc.h \
	rc_hist.c \
	rc_hist.h \
	rc_q2.c \
	rc_q2.h \
	rgb2yuv.c \
	rgb2yuv.h \
	text_bits.c \
	text_bits.h \
	text_code.c \
	text_code.h \
	text_code_mb.c \
	text_code_mb.h \
	text_dct.c \
	text_dct.h \
	text_defs.h \
	vlc.h \
	vm_common_defs.h \
	vm_enc_defs.h \
	vop_code.c \
	vop_code.h \
	zigzag.h

AM_CFLAGS = -D_REENTRANT -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

