if HAVE_LIBMPEG2
ed1=mpeg2dec
else
ed1=
endif
if NO_XVID
ed2=
else
ed2=xvid
endif
if HAVE_FFMPEG
ed3=ffmpeg
else
ed3=
endif

SUBDIRS = raw h261 $(ed1) $(ed2) $(ed3)
