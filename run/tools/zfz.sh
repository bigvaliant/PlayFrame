#!/bin/sh

HADD=1
while [ $HADD -lt 4 ]
do
    sh restart_all.sh
done
