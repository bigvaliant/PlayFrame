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

