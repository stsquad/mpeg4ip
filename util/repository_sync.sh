#! /bin/sh

repository=mpeg4ip

export CVS_RSH=ssh
CVS_N=

release=0
startfrombase=0
skipversion=0
while getopts "rbs" opt; do
    case $opt in
       r ) release=1 ;;
       b ) startfrombase=1 ;;
       s ) skipversion=1 ;;
    esac
done

#
# original repository information.
# origin_dir is local directory where to store
# origin_name is username to use when checking out
# origin_host is name of host where repository is stored
# origin_cvsroot is location of cvs root on host where repository is stored
#
origin_dir=origin
origin_name=wmay
origin_host=buhund
origin_cvsroot=/vws/pan

#
# destination repository information.  See origin information for details
#
dest_dir=sourceforge
dest_name=wmaycisco
dest_host=cvs.mpeg4ip.sourceforge.net
dest_cvsroot=/cvsroot/mpeg4ip

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

origin_CVSROOT="$origin_name@$origin_host:$origin_cvsroot"
dest_CVSROOT="$dest_name@$dest_host:$dest_cvsroot"

#
# create repository directory - make sure it's not appended to the end of
# the directory name already
#
temp=`basename $origin_dir`
if test $repository = $temp; then
   origin_rep=$origin_dir
   origin_dir=`dirname $origin_dir`
else
   origin_rep="$origin_dir/$repository"
fi

#
# same with destination
#
temp=`basename $dest_dir`
if test $repository = $temp; then
   dest_rep=$dest_dir
   dest_dir=`dirname $dest_dir`
else
   dest_rep="$dest_dir/$repository"
fi

# File descriptor usage
# 6 messages and results
# 5 log

exec 5>$temp_dir/repository_updater.log
exec 6>&1

echo "Repository Updater log `date`" >&5
echo Checking for origin repository existance  >&5
echo Checking for origin repository existance  >&6

#
# create or update the origin repository. At this time, update
# origin_dir and origin_rep to have absolute path names
#
if test ! -d $origin_rep; then
    echo $origin_rep does not exist >&5
    echo $origin_rep does not exist >&6
    echo Creating $origin_dir >&5
    mkdir -p $origin_dir >&5
    if test ! -d $origin_dir; then
	exit -1
    fi
    echo Checking out origin cvs repository $origin_CVSROOT >&6
    cd $origin_dir
    origin_dir=`pwd`
    cvs -d $origin_CVSROOT checkout -P -kk $repository 1>&5 2>&5
    cd $repository
    origin_rep=`pwd`
else
    cd $origin_dir
    origin_dir=`pwd`
    cd $repository
    origin_rep=`pwd`
    echo Updating origin cvs repository $origin_rep >&6
    cvs update -P -kk -d 1>&5 2>&5
fi

#
# Do version number update
#
if [ $skipversion = 0 ]; then
   if [ $release = 1 ]; then
      newver=`cat RELEASE_VERSION | tr '.' ' '`
      sh util/version.sh $newver > RELEASE_VERSION
      cp RELEASE_VERSION VERSION
   else
      if [ $startfrombase = 1 ]; then
         newver=`cat RELEASE_VERSION`
         newver=$newver.1
         echo $newver > VERSION
      else
         newver=`cat VERSION | tr '.' ' '`
         sh util/version.sh $newver > VERSION
      fi
   fi
   #
   # Update version file for windows
   #
   echo New version number is `cat VERSION` >&5
   echo New version number is `cat VERSION` >&6
   echo \#define MPEG4IP_PACKAGE \"mpeg4ip\" > include/mpeg4ip_version.h
   echo \#define MPEG4IP_VERSION \"`cat VERSION`\" >> include/mpeg4ip_version.h
   sh util/version_defines.sh `cat VERSION | tr '.' ' '` >>include/mpeg4ip_version.h
   #
   # Update configure.in version
   #
   awk -v VERSION=`cat VERSION` -f util/replaceversion configure.in > temp
   mv temp configure.in

   cvs $CVS_N commit -m 'Version bump for sync' VERSION RELEASE_VERSION include/mpeg4ip_version.h configure.in 1>&5 2>&5
fi

#
# Tag this directory
#
tagvalue=`cat VERSION | tr -s '.' '_'`
tagvalue1=`date +_%y%m%d_%H%M`
tagvalue=VERSION_$tagvalue$tagvalue1

echo Tagging source tree with $tagvalue >&5
echo Tagging source tree with $tagvalue >&6

cvs $CVS_N tag $tagvalue 1>&5 2>&5
cd $start_dir
#
# checkout or update destination repository.  Update dest_dir, dest_rep
# to use absolute path names, as well
#
if test ! -d $dest_rep; then
    echo $dest_rep does not exist >&5
    echo $dest_rep does not exist >&6
    echo Creating $dest_dir >&5
    mkdir -p $dest_dir >&5
    if test ! -d $dest_dir; then
	exit -1
    fi
    echo Checking out destination cvs repository $dest_CVSROOT >&6
    cd $dest_dir
    dest_dir=`pwd`
    cvs -d $dest_CVSROOT checkout -P -kk $repository 1>&5 2>&5
    cd $repository
    dest_rep=`pwd`
else
    cd $dest_dir
    dest_dir=`pwd`
    cd $repository
    dest_rep=`pwd`
    echo Updating dest cvs repository $dest_rep >&6
    cvs update -P -kk -d 1>&5 2>&5
fi

echo "Creating directory lists" >&5
echo "Creating directory lists" >&6
cd $dest_dir
find . -type d -print | grep -v "CVS$" > $temp_dir/dest_dirs

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
  if test $orig_dir_on = "."; then
     continue;
  fi
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
   if test $dest_dir_on = "."; then 
      continue;
   fi
   cd $dest_dir/$dest_dir_on

   if test ! -d $origin_dir/$dest_dir_on; then
      echo $origin_dir/$dest_dir_on no longer exists >&5
      echo $origin_dir/$dest_dir_on no longer exists >&6
      rm * 1>&5 2>&5
      cd ..
      base=`basename $dest_dir_on`
      cat > $temp_dir/commit.note <<EOF
Removing directory $base from repository `date`
EOF
      cvs $CVS_N remove $base 1>&5 2>&5
      cvs $CVS_N commit -F $temp_dir/commit.note $base 1>&5 2>&5
   else
      find . -maxdepth 1 -type f -print > $temp_dir/dest_files
      for file in `cat $temp_dir/dest_files`
      do
         if test ! -e $origin_dir/$dest_dir_on/$file; then
            echo $origin_dir/$dest_dir_on/$file no longer exists
	    rm -f $file
	    cvs $CVS_N remove $file 1>&5 2>&5
	 fi
      done
   fi
done
   
