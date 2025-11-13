#!/bin/bash

echo "Designed Experiments"

echo "Total runtime comparison: CPU vs CnM"

# echo "HBM"

make all SUFFIX=HBM
make all_stats SUFFIX=HBM

echo "DDR4"

sed -i "s/#define DRAM 0/#define DRAM 1/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
make all SUFFIX=DDR4
make all_stats SUFFIX=DDR4

echo "GDDR5"

sed -i "s/#define DRAM 1/#define DRAM 2/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
make all SUFFIX=GDDR5
make all_stats SUFFIX=GDDR5

echo "LPDDR4"

sed -i "s/#define DRAM 2/#define DRAM 3/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
make all SUFFIX=LPDDR4
make all_stats SUFFIX=LPDDR4

sed -i "s/#define DRAM 3/#define DRAM 0/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h


# echo "HBM"

# make SSDResNet34 SUFFIX=HBM

# echo "DDR4"

# sed -i "s/#define DRAM 0/#define DRAM 1/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
# make SSDResNet34 SUFFIX=DDR4

# echo "GDDR5"

# sed -i "s/#define DRAM 1/#define DRAM 2/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
# make SSDResNet34 SUFFIX=GDDR5

# echo "LPDDR4"

# sed -i "s/#define DRAM 2/#define DRAM 3/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h
# make SSDResNet34 SUFFIX=LPDDR4

# sed -i "s/#define DRAM 3/#define DRAM 0/g" ../../../cnm-framework/pim-cores/cnmlib/defs.h