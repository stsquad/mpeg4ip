bin_PROGRAMS = mp4vhdr

INCLUDES = -I$(top_srcdir)/include 
if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif

LDADD = -lm $(LDADD_E1)
