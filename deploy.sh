#!/bin/sh

cur_dir=$(cd `dirname $0`; pwd)
echo $cur_dir

sh ./run/tools/stop_all.sh
sh ./run/tools/log_delete.sh
touch ./run/tools/rsync.txt

rsync -aSvH run/ root@0.0.0.0::game --exclude=.svn --exclude="*.pid" --exclude="*.log" --exclude=".swap" --exclude="config.lua" --exclude="start.sh"
