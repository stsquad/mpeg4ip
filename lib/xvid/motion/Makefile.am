SUBDIRS = x86_asm

noinst_LTLIBRARIES = libxvidmotion.la

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
