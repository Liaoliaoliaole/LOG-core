#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

if command -v dpkg-architecture >/dev/null 2>&1; then
  HOST_MULTIARCH="$(dpkg-architecture -qDEB_HOST_MULTIARCH)"
else
  HOST_MULTIARCH="$(gcc -dumpmachine)"
fi

INSTALL_PREFIX="/usr/local"
INSTALL_LIBDIR="${INSTALL_PREFIX}/lib/${HOST_MULTIARCH}"
export PKG_CONFIG_PATH="${INSTALL_LIBDIR}/pkgconfig:${INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

echo "#Rebuild LOG core code only (no third-party dependency rebuild)"
echo "#Branch: $(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"
echo "#Commit: $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"

# Keep src/sdaq-worker in sync with the recorded submodule pointer and (re)build
# it whenever its own source changed. Both steps are no-ops when nothing
# changed: git skips the checkout if the pointer is already correct, and make
# reports "Nothing to be done" if the sdaq-worker sources are unchanged. This
# runs unconditionally so this script never needs to "detect" whether
# sdaq-worker changed before deciding to update it.
git submodule sync --recursive -- src/sdaq-worker
git submodule update --init --recursive -- src/sdaq-worker
echo "#sdaq-worker submodule: $(git -C src/sdaq-worker rev-parse --short HEAD 2>/dev/null || echo unknown)"

(
  cd src/sdaq-worker
  make tree
  make -j"$(nproc)"
  sudo make install
)

make clean >/dev/null 2>&1 || true
make tree
make -j"$(nproc)"
sudo make install
sudo ldconfig

sudo systemctl restart Morfeas_system.service
sleep 2

echo "#Service status"
systemctl is-active Morfeas_system.service

echo "#Process status"
ps -ef | egrep 'Morfeas_daemon|Morfeas_opc_ua|Morfeas_SDAQ_if' | grep -v egrep || true

echo "#Runtime linkage"
ldd /usr/local/bin/Morfeas_opc_ua | egrep 'open62541|cjson|nopoll|icu|ssl' || true
