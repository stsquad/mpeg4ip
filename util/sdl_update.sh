#! /bin/sh

export CVS_RSH=cvsssh
CVS_N=

#
# original repository information.
# origin_dir is local directory where to store
# origin_name is username to use when checking out
# origin_host is name of host where repository is stored
# origin_cvsroot is location of cvs root on host where repository is stored
#
origin_dir=SDL-1.2.2

#
# destination repository information.  See origin information for details
#
dest_dir=mpeg4ip/lib/SDL

#
# temp_dir is directory where we store files we create, logs
#
temp_dir=temp
start_dir=`pwd`

# Create temp directory
if test -d $temp_dir; then
  mkdir -p $temp_dir
  temp_dir=$start_dir/$temp_dir
fi

cd $dest_dir
dest_dir=`pwd`
cd $start_dir
cd $origin_dir
origin_dir=`pwd`
cd $start_dir

# File descriptor usage
# 6 messages and results
# 5 log

exec 5>$temp_dir/sdl_updater.log
exec 6>&1

echo "SDL Updater log `date`" >&5

cd $dest_dir
find . -type d -print | grep -v "CVS$" > $temp_dir/dest_dirs

cd $start_dir
cd $origin_dir
find . -type d -print | grep -v "CVS$" > $temp_dir/orig_dirs


echo Beginning origin to destination update >&5
echo Beginning origin to destination update >&6

#
# Main loop.  Go through list of directories in origin, 
#

DIR_CNT=0
FILE_CNT=0
NEW_DIR_CNT=0
NEW_FILE_CNT=0
for orig_dir_on in `cat $temp_dir/orig_dirs`
do
  dest_dir_on=$dest_dir/$orig_dir_on
  #
  # check if origin directory exists on destination
  #
  if test ! -d $dest_dir_on; then
    echo $orig_dir_on exists in origin but not destination - creating >&5
    base=`basename $dest_dir_on`
    dir=`dirname $dest_dir_on`
    cd $dir
    mkdir $base
    cvs $CVS_N add $base 1>&5 2>&5
    NEW_DIR_CNT=$(($NEW_DIR_CNT + 1))
  fi

  #
  # Now check all the files in origin directory against files on destination
  #
  cd $origin_dir/$orig_dir_on
  #echo finding files in $orig_dir_on
  find . -maxdepth 1 -type f -print > $temp_dir/orig_files
  FILE_IN_DIR=0
  for file in `cat $temp_dir/orig_files`
  do
    cd $origin_dir/$orig_dir_on
    if test ! -e $dest_dir_on/$file; then
       echo $dest_dir_on/$file does not exist - copying >&5
       cp $file $dest_dir_on
       cd $dest_dir_on
       cvs $CVS_N add $file 1>&5 2>&5
       NEW_FILE_CNT=$(($NEW_FILE_CNT + 1))
       FILE_IN_DIR=$(($FILE_IN_DIR + 1))
    else
       diff=`diff --brief $file $dest_dir_on/$file`
       if test ! -z "$diff"; then
          echo $dest_dir_on/$file has changed - copying >&5
	  diff -c -w $file $dest_dir_on/$file >>$temp_dir/diffs
	  cp $file $dest_dir_on/$file
	  FILE_IN_DIR=$(($FILE_IN_DIR + 1))
       fi
    fi
  done
  FILE_CNT=$(($FILE_CNT + $FILE_IN_DIR))
  if test $FILE_IN_DIR -ne 0; then
    DIR_CNT=$(($DIR_CNT + 1))
  fi
done
rm -f $temp_dir/orig_files

echo Files changed: $FILE_CNT >&6
echo Directories changed: $DIR_CNT >&6
echo New Files added: $NEW_FILE_CNT >&6
echo New Directories added: $NEW_DIR_CNT >&6

echo Checking for removed files from origin >&5
echo Checking for removed files from origin >&6

#
# Now, check for any files in the destination repository that are
# no longer present in the repository.
#
for dest_dir_on in `cat $temp_dir/dest_dirs`
do
   cd $dest_dir/$dest_dir_on

   if test ! -d $origin_dir/$dest_dir_on; then
      echo $origin_dir/$dest_dir_on no longer exists >&5
      echo $origin_dir/$dest_dir_on no longer exists >&6
#      rm * 1>&5 2>&5
#      cd ..
#      base=`basename $dest_dir_on`
#      cat > $temp_dir/commit.note <<EOF
#Removing directory $base from repository `date`
#EOF
#      cvs $CVS_N remove $base 1>&5 2>&5
#      cvs $CVS_N commit -F $temp_dir/commit.note $base 1>&5 2>&5
   else
      find . -maxdepth 1 -type f -print > $temp_dir/dest_files
      for file in `cat $temp_dir/dest_files`
      do
         if test ! -e $origin_dir/$dest_dir_on/$file; then
            echo $origin_dir/$dest_dir_on/$file no longer exists
#	    rm -f $file
#	    cvs $CVS_N remove $file 1>&5 2>&5
	 fi
      done
   fi
done
   
