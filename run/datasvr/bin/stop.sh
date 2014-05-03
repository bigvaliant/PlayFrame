#!/bin/sh

cd `dirname $0`
process_name="datasvr"

./$process_name --conf-file=../conf/config.lua --log-path=../log --log-level=info -D stop
echo "$process_name stop..."

pid=`pgrep $process_name`
echo "$process_name pid: $pid"
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
