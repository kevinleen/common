#!/bin/bash

if [ -n "$1" ]
then
  cd .. && scons -j8 && shift && cd -
fi

bin=../../../../build/redis_cluster


args="-alsolog2stderr"

rm -rf /tmp/*
gdb --ex run --args \
  $bin $args
