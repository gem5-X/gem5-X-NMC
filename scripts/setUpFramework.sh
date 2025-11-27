#!/bin/bash

source scripts/export_paths.sh

# Build m5 directory

cd /CrossLayerNMC/gem5-x-nmc/util/m5 && make -f Makefile.aarch64
cd /CrossLayerNMC

# Install systemc only if not installed
if [ ! -d "$SYSTEMC_HOME" ]; then
    ./scripts/install_systemC.sh
fi

# Install ramulator only if not installed
if [ ! -f "$RAMULATOR_DIR" ]; then
    ./scripts/install_ramulator.sh
fi

# Clone Eigen (Branch we used)
if [ ! -f "$SOFTWARELIB/eigen" ]; then
    ./scripts/install_eigen.sh
fi


