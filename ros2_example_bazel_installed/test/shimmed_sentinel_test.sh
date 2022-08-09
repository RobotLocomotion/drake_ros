#!/bin/bash

output=$($1)

echo $1
echo First test
echo $output

if [ "$output" != "shimmed: yes AMENT_PREFIX_PATH present: yes" ]; then
  exit 1
fi

echo Second test
output=$(_BAZEL_ROS2_RULES_SHIMMED=1 "$1")

echo $output

if [ "$output" != "shimmed: yes AMENT_PREFIX_PATH present: no" ]; then
  exit 1
fi
