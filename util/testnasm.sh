#
# shell to verify nasm version
# nasm -r has format "NASM version <foo> <extra stuff>"
#
# This shell looks for version, then sees if we're 0.98.19 or greater
#
VER=`echo $1 | tr '[a-z]' '[A-Z]'`
until test $VER = "VERSION"; 
  do
   shift
   VER=`echo $1 | tr '[a-z]' '[A-Z]'`
  done

# check for version tag
if test $VER != "VERSION"; then
  echo "no"
  exit 0
fi

shift
if test $1 -gt 0; then 
   echo "yes"
   exit 0
fi

shift
if test $1 -gt 98; then
   echo "yes"
   exit 0
fi

shift
if test -z $1; then
   echo "no"
   exit 0
fi
if test $1 -ge 19; then 
  echo "yes"
  exit 0
fi

echo "no"
exit 0
