#######################################
# @file start.sh
# @brief 启动脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

cd `dirname $0`
process_name="datasvr"

./$process_name --conf-file=../conf/config.lua --log-path=../log --log-level=info -D start 
echo "$process_name start..."

while true
do
    pid=`pgrep $process_name`
    if [[ $pid = "" ]]; then
        echo "..."
        sleep 1
    else
        echo "$process_name start ok!";
        break
    fi
done
