if USE_MMX
SUBDIRS = x86_asm
endif

noinst_LTLIBRARIES = libxvidmotion.la
libxvidmotion_la_LDFLAGS = -static

AM_CFLAGS= -O3

INCLUDES=-I$(top_builddir)/include
libxvidmotion_la_SOURCES = \
	motion_comp.c \
	motion_est.c \
	sad.c \
	sad.h

if USE_MMX
libxvidmotion_la_LIBADD = x86_asm/libxvidmotionx86.la
endif
