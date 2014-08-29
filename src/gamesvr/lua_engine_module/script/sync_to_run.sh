#######################################
# @file sync_to_run.sh
# @brief 同步脚本到运行目录 
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-05-03
#######################################

#!/bin/sh
cd `dirname $0`

rm -rf ../../../../run/gamesvr/script/*

filelist=`ls *.lua`
for file in $filelist; do
    ../../../common/dep/lua/bin/luac -o ../../../../run/gamesvr/script/$file $file
done

cp *.so ../../../../run/script/gamesvr/

