#!/bin/sh
# Script to change the C++ suffix of more than one file at once, # from file.cpp  to file.cc  or file.hpp  to file.hh . 
# Additionally applies ‘dos2unix’ to these files as well as other files, # if you replace the ‘mv’ command below by ‘dos2unix’.
# Additionally runs ‘sed’ script to change ‘.hpp’ include filenames # inside files into to their ‘.hh’ counterparts.
#
# The arguments must be file-names, not directories.
#
# Peter Van Beek, Tue May 14 15:41:33 1996

# !!! commands to apply
#comm1=mv -f
comm1=dos2unix
comm2= sed s/^#include\(.*\)[.]hpp/#include\1.hh/g 


if [ $# = 0 ]
then
echo Usage: cpptocc [filenames] 
exit 1
fi

TMPfile= /tmp/cpptocc.$$ 
# For each file in the argument list...
for file in $*
do
#	echo $file
	# Ignore if not a file
	if [ -f $file ] ; then
		dname= ‘dirname $file‘ 

		# Construct cpp/hpp names
		if [ $dname  != .  ] ; then
			cppbase= $dname/‘basename $file .cpp‘ 
			hppbase= $dname/‘basename $file .hpp‘ 
		else
			cppbase= ‘basename $file .cpp‘ 
			hppbase= ‘basename $file .hpp‘ 
		fi

		# Match names and apply appropriate commands
		if [ $file  = $cppbase.cpp  ] ; then
			echo cpptocc $file $cppbase.cc 
#			echo $comm1 $file $TMPfile 
			$comm1 $file $TMPfile
			echo   >> $TMPfile
#			echo $comm2 < $TMPfile > $cppbase.cc 
			$comm2 < $TMPfile > $cppbase.cc
else 
if [ $file  = $hppbase.hpp  ] ; then
echo cpptocc $file $hppbase.hh 
#				echo $comm1 $file $TMPfile 
				$comm1 $file $TMPfile
				echo   >> $TMPfile
#				echo $comm2 < $TMPfile > $hppbase.hh 
				$comm2 < $TMPfile > $hppbase.hh
			else
				echo cpptocc $file $file 
#				echo $comm1 $file $TMPfile 
				$comm1 $file $TMPfile
				echo   >> $TMPfile
#				echo $comm2 < $TMPfile > $file 
				$comm2 < $TMPfile > $file
			fi
		fi
fi
done
rm -f $TMPfile
exit 0