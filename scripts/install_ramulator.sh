#!/bin/bash

cd $CROSSLAYER_FW

# git clone https://github.com/CMU-SAFARI/ramulator.git

# Update/Patch ramulator files
cp ramulator_files/src/* ramulator/src/
cp ramulator_files/configs/* ramulator/configs/
cp ramulator_files/dram_trace_gen.cpp ramulator/

# Copy them in gem5-x-nmc directory to enable using Ramulator in gem5 simulations
cp -rf ramulator $GEM5_X_NMC/ext/ramulator/Ramulator2

# Build ramulator to use for non FS simulations
cd ramulator
make CXX=g++ -j $(nproc)

cd ..