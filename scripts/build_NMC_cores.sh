#!/bin/bash

cd $ANEMOS/build

# build DRAM=0 (HBM)
make all 
mv nmc-cores /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-HBM
make clean

# build DRAM=1 (DDR4)
sed -i "s/#define DRAM    0/#define DRAM    1/g" ../src/defs.h
make all 
mv nmc-cores /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-DDR4
make clean

# build DRAM=2 (GDDR5)
sed -i "s/#define DRAM    1/#define DRAM    2/g" ../src/defs.h
make all 
mv nmc-cores /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-GDDR5
make clean

# build DRAM=3 (LPDDR4)
sed -i "s/#define DRAM    2/#define DRAM    3/g" ../src/defs.h
make all 
mv nmc-cores /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-LPDDR4

make clean
sed -i "s/#define DRAM    3/#define DRAM    0/g" ../src/defs.h