#/bin/sh
echo \#define MPEG4IP_MAJOR_VERSION 0x$1
echo \#define MPEG4IP_MINOR_VERSION 0x$2
if test -n $3; then
  echo \#define MPEG4IP_CVS_VERSION 0x$3
else
  echo \#define MPEG4IP_CVS_VERSION 0x0
fi
echo \#define MPEG4IP_HEX_VERSION \(\(MPEG4IP_MAJOR_VERSION \<\< 16\) \| \(MPEG4IP_MINOR_VERSION \<\< 8\) \| MPEG4IP_CVS_VERSION\)
