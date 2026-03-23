#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

git submodule sync --recursive
git submodule update --init --recursive

PYTHON3_BIN="$(command -v python3 || true)"
if [ -z "$PYTHON3_BIN" ]; then
  echo "ERROR: python3 not found in PATH" >&2
  exit 1
fi

# Build cJSON
#-------------
echo "#Build cJSON libs."
cd src/cJSON
mkdir -p build
cd build
cmake -D BUILD_SHARED_LIBS=ON ..
make -j"$(nproc)"
sudo make install
sudo ldconfig
cd "$ROOT_DIR"
echo "#Build cJSON libs succeeded."

# Build noPoll
#-------------
echo "#Build noPoll libs."
cd src/noPoll
./autogen.sh
make -j"$(nproc)"
sudo make install
sudo ldconfig
cd "$ROOT_DIR"
echo "#Build noPoll libs succeeded."

# Build open62541
#----------------
echo "#Build open62541 libs."
cd src/open62541
rm -rf build
mkdir -p build
cd build
cmake -D BUILD_SHARED_LIBS=ON -D Python3_EXECUTABLE="$PYTHON3_BIN" ..
make -j"$(nproc)"
sudo make install
sudo ldconfig
cd "$ROOT_DIR"
echo "#Build open62541 libs succeeded."

# Build sdaq-worker
#------------------
echo "#Build sdaq-worker libs."
cd src/sdaq-worker
make tree
make -j"$(nproc)"
sudo make install
cd "$ROOT_DIR"
echo "#Build sdaq-worker libs succeeded."

# Build LOG core
#--------------
echo "#Build LOG core."
make tree
make -j"$(nproc)"
sudo make install
sudo ldconfig
