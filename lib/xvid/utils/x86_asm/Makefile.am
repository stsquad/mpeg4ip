###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidutilsx86.la
libxvidutilsx86_la_LDFLAGS = -static

libxvidutilsx86_la_SOURCES = \
	cpuid.asm \
	mem_transfer_mmx.asm

am_libxvidutilsx86_la_OBJECTS = \
	cpuid.lo \
	mem_transfer_mmx.lo

