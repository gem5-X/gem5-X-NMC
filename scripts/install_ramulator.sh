#!/bin/bash

cd $CROSSLAYER_FW

git clone https://github.com/CMU-SAFARI/ramulator.git
#@TODO need to check which ramulator files to keep
cp ramulator_files/src/* ramulator/src/
cp ramulator_files/configs/* ramulator/configs/
cd ramulator
make CXX=g++ -j $(nproc)

cd ..