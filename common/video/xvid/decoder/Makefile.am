if USE_MMX
SUBDIRS = intel_mmx
endif

noinst_LTLIBRARIES = libxviddecoder.la
libxviddecoder_la_LDFLAGS = -static

libxviddecoder_la_SOURCES = \
	bitreader.h \
	bitreader.c \
	decoder.h \
	decoder.c \
	mbdecoding.h \
	mbdecoding.c \
	mbdeprediction.h \
	mbdeprediction.c

if USE_MMX
libxviddecoder_la_LIBADD = intel_mmx/libxviddecodermmx.la
endif
