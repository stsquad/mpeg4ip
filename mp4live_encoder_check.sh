#!/bin/sh

if grep HAVE_MP4LIVE mpeg4ip_config.h | grep define > /dev/null; then
  echo
else
   echo "MP4Live is not installed (requires V4L2 - check log)"
   exit
fi

if grep HAVE_GTK mpeg4ip_config.h | grep define > /dev/null; then
   echo
else 
   echo "You do not have GTK libraries installed; there will be no mp4live gui"
fi

echo
echo "Mp4live encoder report:"
have_ffmpeg=no
have_xvid=no
have_x264=no
have_lame=no
have_twolame=no
have_faac=no
have_one=no
if grep HAVE_FFMPEG mpeg4ip_config.h | grep define > /dev/null; then
    echo "    ffmpeg encoder is installed"
    have_one=yes
else 
    echo "*** ffmpeg encoder is not installed"
fi
if grep HAVE_XVID mpeg4ip_config.h | grep define > /dev/null; then
    echo "    xvid encoder is installed"
    have_one=yes
else
    echo "*** xvid encoder is not installed"
fi
if grep HAVE_X264 mpeg4ip_config.h | grep define > /dev/null; then
    echo "    x264 encoder is installed"
    have_one=yes
else
    echo "*** x264 encoder is not installed"
fi
if grep HAVE_LAME mpeg4ip_config.h | grep define > /dev/null; then
   echo "    lame encoder is installed"
   have_one=yes
else
   echo "*** lame encoder is not installed"
fi
if grep HAVE_FAAC mpeg4ip_config.h | grep define > /dev/null; then
   echo "    faac encoder is installed"
   have_one=yes
else
   echo "*** faac encoder is not installed"
fi
if grep HAVE_TWOLAME mpeg4ip_config.h | grep define > /dev/null; then
   echo "    twolame encoder is installed"
   have_one=yes
else
   echo "*** twolame encoder is not installed"
fi

if test have_one = "no"; then
   echo 
   echo "There are no encoders installed other than the native H.261 and G.711 encoders"
   echo "If you wish other encoders, please see doc/MAIN_README.html, then rerun bootstrap"
fi
