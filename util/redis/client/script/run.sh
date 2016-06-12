#!/bin/bash

if [ -n "$2" ]
then
  cd .. && scons -j8 && shift && cd -
fi

bin=../../../../build/redis_cluster


args="-alsolog2stderr -key=$1"

rm -rf /tmp/*
#gdb --ex run --args \
  $bin $args
