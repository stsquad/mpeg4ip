lib_LTLIBRARIES = libhttp.la
libhttp_la_LDFLAGS = -static
libhttp_la_SOURCES = \
	http_lib.c 

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -D_REENTRANT -Werror -Wall -Wstrict-prototypes -Wmissing-prototypes

check_PROGRAMS = http
http_SOURCES = http.c
http_LDADD = libhttp.la
