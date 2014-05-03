#!/bin/sh

cd `dirname $0`

if [ -f "rsync.txt" ]; then
    ./restart_all.sh
    sleep 1
    rm rsync.txt
fi 
