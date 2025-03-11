#!/bin/bash

set -e

git submodule update --init --recursive

# Build submodules
echo "#Build cJSON libs."
cd src/cJSON
mkdir -p build && cd build
cmake -D BUILD_SHARED_LIBS=ON ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../../..
echo "#Build cJSON libs succeeded."

echo "#Build noPoll libs."
cd src/noPoll
./autogen.sh
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..
echo "#Build noPoll libs succeeded."

echo "#Build open62541 libs."
cd src/open62541
mkdir -p build && cd build
cmake -D BUILD_SHARED_LIBS=ON ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../../..
echo "#Build open62541 libs succeeded."

echo "#Build sdaq_worker libs."
cd src/sdaq-worker
make tree
make -j$(nproc)
sudo make install
cd ../..

# Build LOG core
echo "Build LOG core."
make tree
make -j$(nproc)
sudo make install
