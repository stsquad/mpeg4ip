EXTRA_DIST=cbp_ppc.s cbp_altivec.s
noinst_LTLIBRARIES = libxvidbitstreamppc.la

libxvidbitstreamppc_la_LDFLAGS = -static
libxvidbitstreamppc_la_SOURCES = dummy.c

if USE_PPC

EXTRA_libxvidbitstreamppc_la_SOURCES = \
	cbp_ppc.s

endif
