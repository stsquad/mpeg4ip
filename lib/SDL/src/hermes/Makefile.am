###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

# The hermes library target
noinst_LTLIBRARIES = libhermes.la

EXTRA_libhermes_la_SOURCES = $(PORTABLE_SRCS)
libhermes_la_LIBADD = \
	mmx_main.lo \
	mmxp2_32.lo \
	x86_main.lo \
	x86p_16.lo \
	x86p_32.lo 

libhermes_la_SOURCES = 

# The hermes library sources
PORTABLE_SRCS = 		\
	mmx_main.asm		\
	mmxp2_32.asm		\
	x86_main.asm		\
	x86p_16.asm		\
	x86p_32.asm		\
	\
	HeadMMX.h		\
	HeadX86.h

EXTRA_DIST =			\
	COPYING.LIB		\
	README

