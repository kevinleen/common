#!/bin/bash

if [ -n "$1" ]
then
  scons -j8
  if [ $? != 0 ]
  then
    echo "build failed..."
    exit -1
  fi
fi

bin=../../../build/kafkaClient

args="-alsolog2stderr \
  -kafka_brokers=192.168.1.252 \
  "

gdb --ex run --args \
  $bin $args
