#
# shell to verify nasm version
# nasm -r has format "NASM version <foo> <extra stuff>"
#
# This shell looks for version, then sees if we're 0.98.19 or greater
#
found_libfaac=no
for i in `cat $1`
do
  word=`echo $i | tr '[a-z]' '[A-Z]'`
  if test $found_libfaac = 'no'; then
     if test $word = "LIBFAAC"; then
        found_libfaac=yes
     fi
  else
     if test $word = "VERSION"; then
        echo "yes"
        exit 0
     fi
     found_libfaac=no
  fi
done
echo no
exit 0

