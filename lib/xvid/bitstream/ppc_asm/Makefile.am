###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

EXTRA_DIST=cbp_ppc.s cbp_altivec.s
if USE_PPC
noinst_LTLIBRARIES = libxvidbitstreamppc.la
libxvidbitstreamppc_la_LDFLAGS = -static

libxvidbitstreamppc_la_SOURCES = \
	cbp_ppc.s

endif
