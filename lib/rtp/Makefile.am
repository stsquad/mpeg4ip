SUBDIRS = win32

noinst_LTLIBRARIES = libuclmmbase.la


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
	boxes-fst.dat \
	btree.h \
	cdecl_ext.h \
	common60.dsp \
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
	rijndael-alg-fst.h \
	rijndael-api-fst.h \
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
	test-libcommon \
	libuclrtp.xcode/project.pbxproj

check_PROGRAMS = test
test_SOURCES = \
	test.c \
	test_base64.c \
	test_base64.h \
	test_des.c \
	test_des.h \
	test_mbus_addr.c \
	test_mbus_addr.h \
	test_mbus_parser.c \
	test_mbus_parser.h \
	test_md5.c \
	test_md5.h \
	test_memory.c \
	test_memory.h \
	test_net_udp.c \
	test_net_udp.h 

test_LDADD = libuclmmbase.la
version.h: VERSION
	sed -e 's/.*/#define CCL_VERSION "v&"/' $? > version.h

AM_CFLAGS = -DDEBUG
