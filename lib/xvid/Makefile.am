SUBDIRS = bitstream dct image motion prediction quant utils

noinst_LTLIBRARIES = libmpeg4ip_xvid.la

AM_CFLAGS= -O3

INCLUDES=-I../../include

libmpeg4ip_xvid_la_SOURCES = \
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

libmpeg4ip_xvid_la_LIBADD = \
	bitstream/libxvidbitstream.la \
	dct/libxviddct.la \
	image/libxvidimage.la \
	motion/libxvidmotion.la \
	prediction/libxvidprediction.la \
	quant/libxvidquant.la \
	utils/libxvidutils.la 
EXTRA_DIST = \
	authors.txt \
	changelog.txt \
	gpl.txt \
	libxvid.dsp \
	todo.txt \
	xvid_st.dsp \
	libxvid.xcode/project.pbxproj

