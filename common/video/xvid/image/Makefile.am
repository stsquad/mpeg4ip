if USE_MMX
SUBDIRS = intel_mmx
endif

noinst_LTLIBRARIES = libxvidimage.la
libxvidimage_la_LDFLAGS = -static

libxvidimage_la_SOURCES = \
	halfpel.h \
	halfpel.c \
	image.h \
	image.c \
	in.h \
	in.c \
	out.h \
	out.c

if USE_MMX
libxvidimage_la_LIBADD = intel_mmx/libxvidimagemmx.la
endif
