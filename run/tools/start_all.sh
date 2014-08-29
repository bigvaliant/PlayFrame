#######################################
# @file start_all.sh
# @brief 启动脚本
# @author fergus <zfengzhen@gmail.com>
# @version 
# @date 2014-08-29
#######################################

#!/bin/sh

cd `dirname $0`

../connsvr/bin/start.sh
../gamesvr/bin/start.sh
../datasvr/bin/start.sh

cd -
ps ux | grep connsvr --color=auto
ps ux | grep gamesvr --color=auto
ps ux | grep datasvr --color=auto
