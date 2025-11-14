#!/bin/bash

source scripts/export_paths.sh

# # Install systemc only if not installed
# if [ ! -d "$SYSTEMC_HOME" ]; then
#     ./scripts/install_systemC.sh
# fi

# # Install ramulator only if not installed
# if [ ! -f "$RAMULATOR_DIR" ]; then
#     ./scripts/install_ramulator.sh
# fi

# # Clone Eigen (Branch we used)
# if [ ! -f "$SOFTWARELIB/eigen" ]; then
#     ./scripts/install_eigen.sh
# fi

# Build m5 directory
docker build -t initbuild .
# didnt work in the server for writing rights 
# docker run -v $PWD:/CrossLayerNMC initbuild \
#     bash -c "cd /CrossLayerNMC/gem5-x-nmc/util/m5 && make --file=Makefile.aarch64"
docker run \
    -u $(id -u):$(id -g) \
    -v $PWD:/CrossLayerNMC \
    initbuild \
    bash -c "cd /CrossLayerNMC/gem5-x-nmc/util/m5 && make -f Makefile.aarch64"
docker rmi -f initbuild
