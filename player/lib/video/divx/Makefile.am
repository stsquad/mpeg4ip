noinst_LTLIBRARIES = libourdivx.la

libourdivx_la_LDFLAGS = -static

libourdivx_la_SOURCES = \
basic_prediction.c \
basic_prediction.h \
global.h \
idct.c \
idct.h \
mp4_block.c \
mp4_block.h \
mp4_decoder.c \
mp4_decoder.h \
mp4_header.c \
mp4_header.h \
mp4_mblock.c \
mp4_mblock.h \
mp4_picture.c \
mp4_predict.c \
mp4_predict.h \
mp4_recon.c \
mp4_vld.c \
mp4_vld.h \
newgetbits.c \
newmain.c \
portab.h \
postprocess.c \
postprocess.h \
transferIDCT.c \
transferIDCT.h \
yuv2rgb.h

AM_CFLAGS = -DMPEG4IP -DPOSTPROCESS -D_REENTRANT -D_DECORE -DLINUX -DO_BINARY=0 -fexceptions @BILLS_CWARNINGS@
