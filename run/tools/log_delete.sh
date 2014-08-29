#######################################
# @file log_delete.sh
# @brief 日志删除脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

cd `dirname $0`

cd ../connsvr/log
rm * -rf

cd -
cd ../gamesvr/log
rm * -rf

cd -
cd ../datasvr/log
rm * -rf

cd -

