#!/bin/bash

if [ $# != 0 ]
then
  scons -j8 
  if [ $? != 0 ]
  then
    exit -1
  fi
fi

bin=../../../build/elastic_search
if [ ! -e $bin ]
then
  echo "no bin"
  exit -1
fi

args="
  -alsolog2stderr \
  -zk_server=192.168.1.250:2181,192.168.1.251:2181,192.168.1.252:2181 \ 
  -es_zk_path="/es/es" -es_zk="es" -es_index="megacorp" -es_types="employee" \
  -es_uri="_search" -test_mode=1 \
"
gdb --ex run --args \
  $bin $args
