if PROVIDE_GETOPTLONG
LIB1 = libgetopt_long.la
endif

noinst_LTLIBRARIES = $(LIB1)

libgetopt_long_la_LDFLAGS = -static
libgetopt_long_la_SOURCES = getopt.c getopt1.c

INCLUDES = -I../../../include

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes
