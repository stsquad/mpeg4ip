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
if test $1 -gt 3; then 
   echo "yes"
   exit 0
fi

if test $1 -lt 2; then
   echo "no"
   exit 0
fi

shift
if test $1 -ge 92; then
   echo "yes"
   exit 0
fi

echo "no"
exit 0
