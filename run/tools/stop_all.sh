#!/bin/sh

cd `dirname $0`
../connsvr/bin/stop.sh
../gamesvr/bin/stop.sh
../datasvr/bin/stop.sh

cd -
ps ux | grep connsvr --color=auto
ps ux | grep gamesvr --color=auto
ps ux | grep datasvr --color=auto
