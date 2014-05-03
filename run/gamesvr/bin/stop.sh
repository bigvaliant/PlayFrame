#!/bin/sh

cd `dirname $0`
process_name="gamesvr"

./$process_name --conf-file=../../conf/gamesvr/config.lua --log-path=../../log/gamesvr --log-level=info -D stop
echo "$process_name stop..."

pid=`pgrep $process_name`
while true
do
    if [ ! -f "/proc/$pid/cwd/$process_name" ]; then
        echo "$process_name stop finished!"
        break
    else
        echo "...";
        sleep 1
    fi
done

ipcrm -M 1222
ipcrm -M 1223
