#!/usr/bin/env bash
# Build this package as the developer, then update and restart its service.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
workspace=$(cd -- "${script_dir}/../../.." && pwd)

cd "${workspace}"
colcon build --symlink-install --packages-select armatron_drive
sudo "${script_dir}/install.sh"
sudo systemctl restart armatron-drive.service
