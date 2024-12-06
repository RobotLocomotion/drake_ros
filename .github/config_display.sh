#!/bin/sh
set -x
sudo apt-get update && sudo apt-get install libgl1-mesa-glx xvfb -y
Xvfb :1 -screen 0 1024x768x24 > /dev/null 2>&1 &
# give xvfb some time to start
sleep 3
set +x
