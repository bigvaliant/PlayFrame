#######################################
# @file restart_all.sh
# @brief 重启脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

cd `dirname $0`

./stop_all.sh
./log_delete.sh
./start_all.sh

cd -
