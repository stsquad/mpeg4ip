#!/bin/sh
INSTALL=install
EXE=$2/$1

if [ ! -f $EXE ]
then
    $INSTALL $1 $2
    echo Installed $1 in $2
#else
#    echo Something
fi
