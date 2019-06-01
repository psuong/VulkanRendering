#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=$1 -H. -Bbin/$1

cd bin/$1
make
cd ..
