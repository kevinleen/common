#!/bin/bash

if [ -n "$1" ]
then
  cd .. && scons -j8 && shift && cd -
fi

bin=../../../../build/zk_proxy

pid=`ps aux | grep test.sh | grep -v "grep" | awk '{print $2}'`
if [ -z "$pid" ]
then
  echo "no server"
  exit -1
fi

args="-alsolog2stderr -watch_pid=$pid \
  -zk_server=192.168.1.250:2181,192.168.1.251:2181,192.168.1.252:2181 \
  -zk_path=/zk/proxy/2 -proxy_server=192.168.1.250:80 \
"

rm -rf /tmp/*
gdb --ex run --args \
  $bin $args
