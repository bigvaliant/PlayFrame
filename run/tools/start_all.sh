#!/bin/sh

cd `dirname $0`

../connsvr/bin/start.sh
../gamesvr/bin/start.sh
../datasvr/bin/start.sh

cd -
ps ux | grep connsvr --color=auto
ps ux | grep gamesvr --color=auto
ps ux | grep datasvr --color=auto
