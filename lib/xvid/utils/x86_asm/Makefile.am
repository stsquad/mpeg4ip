###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL)  --mode=compile @LIBTOOL_C_TAG@ $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libxvidutilsx86.la

ASMFILES = \
	cpuid.asm \
	mem_transfer_mmx.asm

libxvidutilsx86_la_SOURCES = dummy.c

if USE_MMX
EXTRA_libxvidutilsx86_la_SOURCES = $(ASMFILES)

libxvidutilsx86_la_LIBADD = \
	cpuid.lo \
	mem_transfer_mmx.lo
endif

