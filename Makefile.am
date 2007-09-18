if SERVER
if PLAYER
SUBDIRS = include lib common player server util doc 
else
SUBDIRS = include lib common server util doc
endif
else
if PLAYER
SUBDIRS = include lib common player util doc 
else
SUBDIRS = include lib util doc
endif
endif

EXTRA_DIST = bootstrap cvs_bootstrap encoding60.dsw README_FIRST \
	strip_fPIC.sh depcomp README.html faac_mpeg4ip_add.tar \
	mp4liveOnDebian.FAQ \
	index.html FEATURES.html suse90pro.installed mp4live_encoder_check.sh \
	player.sln tools.sln

include_HEADERS = mpeg4ip_config.h

bin_SCRIPTS = mpeg4ip-config
