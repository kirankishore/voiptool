#!/bin/bash

name=`basename $0`       # $0 is the executed command - f.e. "./link.sh"
dir=${0%$name}           # remove the name of the script from the back end of the string - left over is the directory
if [ ! -x $name ]; then  # if the script isn't run from the directory it is placed in (executed: libs/link.sh ) then
cd $dir                  # change to that dir
fi

curdir=`pwd`             # runs the command pwd and saves the result into the variable curdir 

if [ -h libavcodec.so ]; then
rm libavcodec.so
fi
ln -s libavcodec.so.51 libavcodec.so

if [ -h libavformat.so ]; then
rm libavformat.so
fi
ln -s libavformat.so.51 libavformat.so

if [ -h libavutil.so ]; then
rm libavutil.so
fi
ln -s libavutil.so.49 libavutil.so

if [ ! -d ~/lib ]; then    # if the directory doesn't exist, create it
mkdir ~/lib
fi

if [ -h ~/lib/libavcodec.so.51 ]; then  # remove links if they exist to create new one's
rm ~/lib/libavcodec.so.51
fi
ln -s $curdir/libavcodec.so.51 ~/lib/libavcodec.so.51

if [ -h ~/lib/libavformat.so.51 ]; then
rm ~/lib/libavformat.so.51
fi

ln -s $curdir/libavformat.so.51 ~/lib/libavformat.so.51

if [ -h ~/lib/libavutil.so.49 ]; then
rm ~/lib/libavutil.so.49
fi

ln -s $curdir/libavutil.so.49 ~/lib/libavutil.so.49

exit 0
