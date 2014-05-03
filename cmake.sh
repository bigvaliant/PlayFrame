#!/bin/sh

mkdir -p build
cd ./build
rm * -rf
#cmake ../src/ 2>&1 | tee result.log
#make 2>&1 | tee -a result.log

cmake ../src/
make


