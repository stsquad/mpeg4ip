if SERVER
if PLAYER
SUBDIRS = include lib common player server util doc mpeg4ip.pbproj
else
SUBDIRS = include lib common server util doc
endif
else
if PLAYER
SUBDIRS = include lib common player util doc mpeg4ip.pbproj
else
SUBDIRS = include lib util doc
endif
endif

EXTRA_DIST = bootstrap cvs_bootstrap encoding60.dsw README_FIRST strip_fPIC.sh depcomp README.html patch.osx faac_mpeg4ip_add.tar mp4liveOnDebian.FAQ \
mac_build_libs mac_build_tools

include_HEADERS = mpeg4ip_config.h

bin_SCRIPTS = mpeg4ip-config
