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

EXTRA_DIST = bootstrap cvs_bootstrap encoding60.dsw README_FIRST strip_fPIC.sh depcomp README.html
