###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidmotionx86.la
libxvidmotionx86_la_LDFLAGS = -static

libxvidmotionx86_la_SOURCES = dummy.c

ASMFILES = sad_mmx.asm
if USE_MMX
EXTRA_libxvidmotionx86_la_SOURCES = $(ASMFILES)

libxvidmotionx86_la_LIBADD = \
	sad_mmx.lo
endif

EXTRA_DIST = $(ASMFILES)
