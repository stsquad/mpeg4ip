if USE_MMX
SUBDIRS = intel_mmx
endif

noinst_LTLIBRARIES = libxvidcommon.la
libxvidcommon_la_LDFLAGS = -static

libxvidcommon_la_SOURCES = \
	common.h \
	common.c \
	fdct.c \
	idct.c \
	interpolate8x8.c \
	quantize.c \
	quantize4.c \
	timer.h \
	timer.c \
	transfer.c

if USE_MMX
libxvidcommon_la_LIBADD = intel_mmx/libxvidcommonmmx.la
endif
