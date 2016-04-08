#/bin/bash -e

gdb --ex run --args \
  ../../../../build/zkClient  -serv_name="hello" \
    -serv_path="/zk/proxy" -server_finder \
    -zk_server=192.168.1.250:2181,192.168.1.251:2181,192.168.1.252:2181 \
    -alsolog2stderr -dlog_on
