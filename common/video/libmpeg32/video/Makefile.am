###########################################################################
#
# Some consistent rules for building asm files:

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile $(STRIP_FPIC) $(NASM) @NASMFLAGS@ $<

###########################################################################

noinst_LTLIBRARIES = libmpeg3video.la
libmpeg3video_la_LDFLAGS = -static

INCLUDES=-I$(top_srcdir)/include -I$(top_srcdir)/lib/SDL/include
if HAVE_ALIGN_LOOPS
   OUR_ALIGN_LOOPS=@ALIGN_LOOPS@=2
else
   OUR_ALIGN_LOOPS=
endif 
if HAVE_ALIGN_JUMPS
   OUR_ALIGN_JUMPS=@ALIGN_JUMPS@=2
else
   OUR_ALIGN_JUMPS=
endif 
if HAVE_ALIGN_FUNCS
   OUR_ALIGN_FUNCS=@ALIGN_FUNCS@=2
else
   OUR_ALIGN_FUNCS=
endif 

if USE_MMX
AM_CFLAGS = -DHAVE_MMX -fomit-frame-pointer $(OUR_ALIGN_LOOPS) $(OUR_ALIGN_JUMPS) $(OUR_ALIGN_FUNCS)

EXTRA_libmpeg3video_la_SOURCES = \
     mmxidct.S \
     reconmmx.asm 
libmpeg3video_la_LIBADD = \
	mmxidct.lo \
     reconmmx.lo 
else
AM_CFLAGS = -fomit-frame-pointer $(OUR_ALIGN_LOOPS) $(OUR_ALIGN_JUMPS) $(OUR_ALIGN_FUNCS)
EXTRA_libmpeg3video_la_SOURCES = 
libmpeg3video_la_LIBADD = 

endif

libmpeg3video_la_SOURCES = \
	getpicture.c \
	headers.c \
	idct.c \
	idct.h \
	layerdata.h \
	macroblocks.c \
	mmxtest.c \
	motion.c \
	mpeg3video.c \
	mpeg3video.h \
	mpeg3videoprotos.h \
	output.c \
	reconstruct.c \
	seek.c \
	slice.c \
	slice.h \
	vlc.c \
	vlc.h 

EXTRA_DIST = worksheet.c libmpeg32video.dsp

