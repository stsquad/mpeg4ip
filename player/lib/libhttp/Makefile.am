lib_LTLIBRARIES = libhttp.la
libhttp_la_SOURCES = \
	http.c \
	http.h \
	http_private.h \
	http_resp.c \
	http_util.c

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = -D_REENTRANT @BILLS_CWARNINGS@

check_PROGRAMS = http_test post_test
http_test_SOURCES = http_test.c
http_test_LDADD = libhttp.la

post_test_SOURCES = post_test.c
post_test_LDADD = libhttp.la

EXTRA_DIST = libhttp60.dsp 
