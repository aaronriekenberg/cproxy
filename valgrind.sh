#!/bin/bash

BASE_PORT=5000
NUM_PORTS=1
CMD="./cproxy"
i=0
while [ ${i} -lt ${NUM_PORTS} ]; do
  CMD="${CMD} -l 0.0.0.0:$((${BASE_PORT} + ${i}))"
  i=$((${i} + 1))
done
CMD="valgrind --leak-check=full ${CMD} -r 127.0.0.1:10000 -b $((256 * 1024)) -t 2"
echo "${CMD}"
$CMD
