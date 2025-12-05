 #!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: sh $0 <nmc_cores_executable_path> <name_of_gem5_build_instance>"
    echo "Example: sh $0 /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-HBM HBM"
    exit 1
fi

echo $GEM5_X_NMC

cd $GEM5_X_NMC

# Update the path to the executable
sed -i "s|nmc-cores|nmc-cores-$2|g" /CrossLayerNMC/gem5-x-nmc/src/mem/nmccores.cc

# Build gem5-x-nmc
# The name of the build MUST be gem5.fast. 
scons build/ARM/gem5.fast -j64

# Then to have multiple builds, rename the file.
mv build/ARM/gem5.fast build/ARM/gem5_$2.fast

# Set path to default 
sed -i "s|$1|/CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores|g" src/mem/nmccores.cc