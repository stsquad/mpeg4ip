bin_PROGRAMS = oldfaac

oldfaac_SOURCES = faac.c

INCLUDES = -I$(top_srcdir)/include 

AM_CFLAGS = -O2

if HAVE_LIBSNDFILE
LD_E1 = -lsndfile
else
LD_E1 = 
endif 

LDADD = $(LD_E1) \
	-lfaac -lm

