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

libxvidmotionx86_la_SOURCES = \
	sad_mmx.asm

am_libxvidmotionx86_la_OBJECTS = \
	sad_mmx.lo

