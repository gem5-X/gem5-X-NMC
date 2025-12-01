#!/bin/bash

cd $CrossLayerFW

mkdir -p $SYSTEMC_HOME

wget https://github.com/accellera-official/systemc/archive/refs/tags/2.3.3.tar.gz
tar -xzf 2.3.3.tar.gz
cd systemc-2.3.3
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$SYSTEMC_HOME -DCMAKE_CXX_STANDARD=11 ..
make install -j $(nproc)
cd ../..
rm -rf systemc-2.3.3 2.3.3.tar.gz
echo "SystemC installed at $SYSTEMC_HOME"