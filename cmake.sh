#######################################
# @file cmake.sh
# @brief 编译脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

mkdir -p build
cd ./build
rm * -rf
#cmake ../src/ 2>&1 | tee result.log
#make 2>&1 | tee -a result.log

cmake ../src/
make


