
noinst_LTLIBRARIES = libuclmmbase.la

libuclmmbase_la_LDFLAGS = -static

libuclmmbase_la_SOURCES= \
	asarray.c \
	base64.c \
	btree.c \
	crypt_random.c \
	debug.c \
	drand48.c \
	hmac.c \
	inet_ntop.c \
	inet_pton.c \
	mbus.c \
	mbus_addr.c \
	mbus_config.c \
	mbus_parser.c \
	md5.c \
	memory.c \
	net_udp.c \
	ntp.c \
	qfDES.c \
	rijndael-alg-fst.c \
	rijndael-api-fst.c \
	rtp.c \
	util.c \
	version.h 

EXTRA_DIST = \
	acconfig.h \
	addrinfo.h \
	addrsize.h \
	asarray.h \
	base64.h \
	bills_srtp.h \
	bittypes.h \
	btree.h \
	cdecl_ext.h \
	config_unix.h \
	config_win32.h \
	crypt_random.h \
	debug.h \
	drand48.h \
	gettimeofday.h \
	hmac.h \
	inet_ntop.h \
	inet_pton.h \
	mbus_addr.h \
	mbus_config.h \
	mbus.h \
	mbus_parser.h \
	md5.h \
	memory.h \
	net_udp.h \
	ntp.h \
	qfDES.h \
	rijndael-alg-fst.h \
	rijndael-api-fst.h \
	rtp.h \
	sockstorage.h \
	test_base64.h \
	test_des.h \
	test_mbus_addr.h \
	test_mbus_parser.h \
	test_md5.h \
	test_memory.h \
	test_net_udp.h \
	util.h \
	vsnprintf.h \
	boxes-fst.dat

version.h: VERSION
	sed -e 's/.*/#define CCL_VERSION "v&"/' $? > version.h

AM_CFLAGS = -DDEBUG
