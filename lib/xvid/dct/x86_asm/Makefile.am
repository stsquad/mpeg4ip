###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxviddctx86.la
libxviddctx86_la_LDFLAGS = -static

libxviddctx86_la_SOURCES = \
	fdct_mmx.asm \
	idct_mmx.asm

am_libxviddctx86_la_OBJECTS = \
	fdct_mmx.lo \
	idct_mmx.lo

