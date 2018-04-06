#!/bin/bash

dir=`pwd`/`dirname $0`
cd $dir
mkdir -p build
cd build
cmake ..
make

