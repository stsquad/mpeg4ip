if USE_MMX
SUBDIRS = intel_mmx
endif

noinst_LTLIBRARIES = libxvidencoder.la
libxvidencoder_la_LDFLAGS = -static

AM_CFLAGS = -DPMVFAST

libxvidencoder_la_SOURCES = \
	bitwriter.h \
	bitwriter.c \
	cbp.h \
	cbp.c \
	encoder.h \
	encoder.c \
	mbcoding.c \
	mbfunctions.h \
	mbmotionestcomp.c \
	mbprediction.c \
	mbtransquant.c \
	putvlc.h \
	putvlc.c \
	ratecontrol.h \
	ratecontrol.c \
	sad.h \
	sad.c \
	vlc_codes.h

if USE_MMX
libxvidencoder_la_LIBADD = intel_mmx/libxvidencodermmx.la
endif
