###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidbitstreamx86.la
libxvidbitstreamx86_la_LDFLAGS = -static

libxvidbitstreamx86_la_SOURCES = \
	cbp_mmx.asm

am_libxvidbitstreamx86_la_OBJECTS = \
	cbp_mmx.lo

