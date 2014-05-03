#!/bin/sh

cd `dirname $0`
process_name="connsvr"

./$process_name --conf-file=../conf/config.lua --log-path=../log --log-level=info -D stop
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

ipcrm -M 1121
