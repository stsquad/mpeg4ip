
lib_LTLIBRARIES = libuclmmbase.la

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

version.h: VERSION
	sed -e 's/.*/#define CCL_VERSION "v&"/' $? > version.h

AM_CFLAGS = -DDEBUG
