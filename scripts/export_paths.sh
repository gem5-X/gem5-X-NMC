#!/bin/bash

# Needs to be used in the root directory of the project with source scripts/export_paths.sh
export CROSSLAYER_FW=$PWD
export ANEMOS=$CROSSLAYER_FW/ANEMOS
export SOFTWARELIB=$CROSSLAYER_FW/softwareStack
export GEM5_X_NMC=$CROSSLAYER_FW/gem5-x-nmc
export RAMULATOR_DIR=$CROSSLAYER_FW/ramulator  
export SYSTEMC_HOME=$CROSSLAYER_FW/systemc 

# export INPUTS_DIR=$SIDEDRAM_HOME/inputs #@TODO DO I NEED THIS?
# export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH #@TODO DO I NEED THIS?

echo "CrossLayerFW: $CROSSLAYER_FW"


#@TODO WHAT IS THE PURPOSE OF THE LINES BELOW?
# INPUTS_SED=$(echo $INPUTS_DIR | sed 's/\//\\\//g')

# # Substitute INPUTS_DIR with $INPUTS_DIR in the needed files
# sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/inputs/src/gen_gemm_assembly.cpp
# sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/inputs/src/map_kernel.cpp
# sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/src/control_unit.cpp
# sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/src/tb/pch_driver.cpp