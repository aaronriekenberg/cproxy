#!/usr/bin/env bash

BASE_PORT=5000
NUM_PORTS=10
NUM_THREADS=4
CMD="./cproxy"
i=0
while [ ${i} -lt ${NUM_PORTS} ]; do
  CMD="${CMD} -l 0.0.0.0:$((${BASE_PORT} + ${i}))"
  i=$((${i} + 1))
done
CMD="${CMD} -r 127.0.0.1:10000 -b $((256 * 1024)) -t ${NUM_THREADS}"
echo "${CMD}"
$CMD
