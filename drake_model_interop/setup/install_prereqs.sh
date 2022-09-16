#!/bin/bash

set -eux -o pipefail

apt-get update
apt-get install lsb-release wget gnupg

sh -c 'echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list'
wget http://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -
apt-get update
apt-get install ignition-edifice

