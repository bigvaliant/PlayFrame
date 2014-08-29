#######################################
# @file check.sh
# @brief 运行环境运行检测脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

cd `dirname $0`

if [ -f "rsync.txt" ]; then
    ./restart_all.sh
    sleep 1
    rm rsync.txt
fi 
