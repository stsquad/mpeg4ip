bin_PROGRAMS = mp4vpkt
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib/mp4
if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif
LDADD = $(top_builddir)/lib/mp4/libmp4.la -lm $(LDADD_E1)
