bin_PROGRAMS = mp4apkt

mp4apkt_SOURCES = \
	mp4apkt.c \
	aac.c \
	mp3.c

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4

LDADD = $(top_builddir)/lib/mp4/libmp4.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la

