#!/bin/bash

if [ $# != 0 ]
then
  scons -j8 
  if [ $? != 0 ]
  then
    exit -1
  fi
fi

bin=../../../build/http
if [ ! -e $bin ]
then
  echo "no bin"
  exit -1
fi

args="
  -ssl -http_port=443 \
  -alsolog2stderr \
"


gdb --ex run --args \
  $bin $args
