#!/bin/sh

cd `dirname $0`

./stop_all.sh
./log_delete.sh
./start_all.sh

cd -
