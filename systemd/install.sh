#!/usr/bin/env bash
set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
  echo "Run with sudo: sudo ./systemd/install.sh" >&2
  exit 1
fi

unit_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
unit="${unit_dir}/armatron-drive.service"
target="/etc/systemd/system/armatron-drive.service"

if [[ -e ${target} && ! -L ${target} ]]; then
  echo "Refusing to replace non-symlink unit: ${target}" >&2
  exit 1
fi

ln -sfn "${unit}" "${target}"
echo "Linked ${target} -> ${unit}"
systemctl daemon-reload
