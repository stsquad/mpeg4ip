bin_PROGRAMS = avi2raw
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/avi
if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif
LDADD = $(top_builddir)/lib/avi/libavi.la $(LDADD_E1) -lm
