
noinst_LTLIBRARIES = libuclmmbase.la


libuclmmbase_la_SOURCES= \
	debug.c \
	drand48.c \
	inet_ntop.c \
	inet_pton.c \
	memory.c \
	net_udp.c \
	ntp.c \
	rtp.c \
	util.c \
	version.h 
#	btree.c \
#	base64.c \
#	asarray.c \
#	hmac.c \
#	mbus.c \
#	mbus_addr.c \
#	mbus_config.c \
#	mbus_parser.c 
#	crypt_random.c \
#	md5.c \
#	qfDES.c 

EXTRA_DIST = \
	acconfig.h \
	addrinfo.h \
	addrsize.h \
	asarray.h \
	base64.h \
	bittypes.h \
	boxes-fst.dat \
	btree.h \
	cdecl_ext.h \
	common60.dsp rtp_common.vcproj \
	config_unix.h \
	config_win32.h \
	COPYRIGHT \
	crypt_random.h \
	debug.h \
	drand48.h \
	getaddrinfo.c \
	gettimeofday.c \
	gettimeofday.h \
	hmac.h \
	inet_ntop.h \
	inet_pton.h \
	install-sh \
	mbus_addr.h \
	mbus_config.h \
	mbus.h \
	mbus_parser.h \
	md5.h \
	memory.h \
	net_udp.h \
	ntp.h \
	qfDES.h \
	rtp.h \
	sockstorage.h \
	util.h \
	vsnprintf.c \
	vsnprintf.h \
	MODS \
	README \
	README.qfdes \
	VERSION \
	test_rtp_encrypt.txt \
	test-libcommon 

INCLUDES=-I$(top_srcdir)/include -I$(top_srcdir)/lib/utils

check_PROGRAMS = test_rtp_client test_rtp_server

AM_CFLAGS = -DDEBUG -Wall -Werror
test_rtp_client_SOURCES = test_rtp_client.c
test_rtp_client_LDADD = libuclmmbase.la \
	$(top_builddir)/lib/utils/libmutex.la \
	@SRTPLIB@ @SDL_LIBS@
test_rtp_server_SOURCES = test_rtp_server.c
test_rtp_server_LDADD = libuclmmbase.la \
	$(top_builddir)/lib/utils/libmutex.la \
	@SRTPLIB@ @SDL_LIBS@

#check_PROGRAMS = test
#test_SOURCES = \
#	test.c \
#	test_base64.c \
#	test_base64.h \
#	test_des.c \
#	test_des.h \
#	test_mbus_addr.c \
#	test_mbus_addr.h \
#	test_mbus_parser.c \
#	test_mbus_parser.h \
#	test_md5.c \
#	test_md5.h \
#	test_memory.c \
#	test_memory.h \
#	test_net_udp.c \
#	test_net_udp.h 

#test_LDADD = libuclmmbase.la
version.h: VERSION
	sed -e 's/.*/#define CCL_VERSION "v&"/' $? > version.h

