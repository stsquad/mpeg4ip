###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

bin_PROGRAMS = divxenc
divxenc_SOURCES = divxenc.c

INCLUDES = -I$(top_srcdir)/include

LDADD = $(top_builddir)/server/video/divx/libdivxencore.la -lm \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la



AM_CFLAGS = -DMPEG4IP -D_REENTRANT -O3 -Wcast-align -Wstrict-prototypes -Wmissing-prototypes

lib_LTLIBRARIES = libdivxencore.la

if USE_MMX
EXTRA_libdivxencore_la_SOURCES = \
	text_dct.c \
	text_fdct_mmx.asm \
	text_idct_mmx.asm \
	interpolate_mmx.asm \
	sad_mb_mmx.asm
libdivxencore_la_LIBADD = \
	text_dct.lo \
	text_fdct_mmx.lo \
	text_idct_mmx.lo \
	interpolate_mmx.lo \
	sad_mb_mmx.lo
else
EXTRA_libdivxencore_la_SOURCES = \
	text_dct.c
libdivxencore_la_LIBADD = \
	text_dct.lo
endif

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
	rate_ctl.c \
	rate_ctl.h \
	text_bits.c \
	text_bits.h \
	text_code.c \
	text_code.h \
	text_code_mb.c \
	text_code_mb.h \
	text_dct.h \
	text_defs.h \
	vlc.h \
	vm_common_defs.h \
	vm_enc_defs.h \
	vop_code.c \
	vop_code.h \
	zigzag.h 

