#!/bin/sh

CFLAGS="-std=c++11 -Wall"

if [ "$(uname)" == "Darwin" ];
then # OSX
    CFLAGS+=" -F/Users/robin/Library/Frameworks/ -framework SDL2"
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ];
then # LINUX
    echo "Fix config for Linux!"
    exit
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW32_NT" ];
then # WIN
    echo "Fix config for Windows!"
    exit
else
    echo "Could not detect platform!"
    exit
fi

g++ $CFLAGS main.cpp -o game

