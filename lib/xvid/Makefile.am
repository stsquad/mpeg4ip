SUBDIRS = bitstream dct image motion prediction quant utils

lib_LTLIBRARIES = libxvid.la

AM_CFLAGS= -O3

INCLUDES=-I../../include

libxvid_la_SOURCES = \
	decoder.h \
	decoder.c \
	divx4.h \
	divx4.c \
	encoder.h \
	encoder.c \
	global.h \
	portab.h \
	xvid.h \
	xvid.c

libxvid_la_LIBADD = \
	bitstream/libxvidbitstream.la \
	dct/libxviddct.la \
	image/libxvidimage.la \
	motion/libxvidmotion.la \
	prediction/libxvidprediction.la \
	quant/libxvidquant.la \
	utils/libxvidutils.la 

