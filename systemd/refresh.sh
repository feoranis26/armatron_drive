#!/usr/bin/env bash
# Build this package as the developer, then update and restart its service.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
workspace=$(cd -- "${script_dir}/../../.." && pwd)

cd "${workspace}"
ros_setup=${ARMATRON_ROS_SETUP:-/opt/ros/humble/setup.bash}
if [[ ! -f ${ros_setup} ]]; then
  echo "ROS underlay setup not found: ${ros_setup}" >&2
  exit 1
fi
# Do not rebuild against a previously sourced copy of this same workspace.
env -i HOME="${HOME}" USER="$(id -un)" LOGNAME="$(id -un)" \
  LANG="${LANG:-C.UTF-8}" \
  PATH="${HOME}/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
  bash --noprofile --norc -c '
    set -eo pipefail
    source "$1"
    exec colcon build --symlink-install --packages-select armatron_drive
  ' bash "${ros_setup}"
sudo "${script_dir}/install.sh"
sudo systemctl restart armatron-drive.service
