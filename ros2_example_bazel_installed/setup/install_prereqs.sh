#!/bin/bash

set -eux pipefail

apt update && apt install apt-transport-https curl gnupg lsb-release cmake build-essential gettext-base coreutils

as-user() {
  if [[ -n "${SUDO_USER:+D}" ]]; then
    sudo -u ${SUDO_USER} "$@"
  else
    "$@"
  fi
}

# Install Bazel (derived from setup/ubuntu/source_distribution/install_prereqs.sh at
# https://github.com/RobotLocomotion/drake/tree/e91e62f524788081a8fd231129b64ff80607c1dd)
function dpkg_install_from_curl() {
  package="$1"
  version="$2"
  url="$3"
  checksum="$4"

  installed_version=$(dpkg-query --showformat='${Version}\n' --show "${package}" 2>/dev/null || true)
  if [[ "${installed_version}" != "" ]]; then
      # Skip the install if we're already at the exact version.
      if [[ "${installed_version}" == "${version}" ]]; then
          echo "${package} is already at the desired version ${version}"
          return
      fi

      # If installing our desired version would be a downgrade or an upgrade, ask the user first.
      echo "This system has ${package} version ${installed_version} installed."
      action="upgrade"  # Assume an upgrade
      if dpkg --compare-versions "${installed_version}" gt "${version}"; then
          action="downgrade"  # Switch to a downgrade
      fi
      echo "Drake ROS intends to ${action} to version ${version}, the supported version."
      read -r -p "Do you want to ${action}? [Y/n] " reply
      if [[ ! "${reply}" =~ ^([yY][eE][sS]|[yY])*$ ]]; then
          echo "Skipping ${package} ${version} installation."
          return
      fi
  fi
  # Download and verify.
  tmpdeb="/tmp/${package}_${version}-amd64.deb"
  curl -sSL "${url}" -o "${tmpdeb}"
  if echo "${checksum} ${tmpdeb}" | sha256sum -c -; then
    echo  # Blank line between checkout output and dpkg output.
  else
    echo "ERROR: The ${package} deb does NOT have the expected SHA256. Not installing." >&2
    exit 2
  fi

  # Install.
  dpkg -i "${tmpdeb}"
  rm "${tmpdeb}"
}

apt install g++ unzip zlib1g-dev

dpkg_install_from_curl \
  bazel 5.1.0 \
  https://releases.bazel.build/5.1.0/release/bazel_5.1.0-linux-x86_64.deb \
  3d54055f764cfb61b5416f0a45d2d3df19c30d301d4da81565595cbe2e36a220

# TODO(hidmic): install distributions from debians when Drake supports 22.04
if [[ -z "${ROS2_DISTRO_PREFIX:-}" ]]; then
  # Install dependencies for ROS 2 Rolling on Focal tarball
  ROS2_APT_SOURCE="deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main"
  if ! grep "${ROS2_APT_SOURCE}" /etc/apt/sources.list.d/ros2.list; then
    echo ${ROS2_APT_SOURCE} | tee /etc/apt/sources.list.d/ros2.list
  fi

  apt update && apt install python3-rosdep
  [[ -d /etc/ros/rosdep ]] || rosdep init
  as-user rosdep update --rosdistro=rolling

  # TODO(hidmic): be very explicit about what installation mechanisms we allow
  # NOTE: since no ROS distributions has been sourced or specified yet,
  # force Python version to 3.x (which is standard in ROS 2 distributions)
  SETUP_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
  apt install $(env ROS_PYTHON_VERSION=3 rosdep resolve \
    $(cat ${SETUP_DIR}/prereq-rosdep-keys.txt) 2>/dev/null | grep -v '^#') libssl-dev
fi

# Install Python dependencies
apt install python3 python3-toposort python3-dev python-is-python3

# Install debuggers
apt install gdb lldb
