#!/bin/bash

# Needs to be used in the root directory of the project with source scripts/export_paths.sh
export CROSSLAYER_FW=$PWD
export ANEMOS=$CROSSLAYER_FW/ANEMOS
export SOFTWARELIB=$CROSSLAYER_FW/softwareStack
export GEM5_X_NMC=$CROSSLAYER_FW/gem5-x-nmc
export RAMULATOR_DIR=$CROSSLAYER_FW/ramulator  
export SYSTEMC_HOME=$CROSSLAYER_FW/systemc 
export CPP_APPS=$SOFTWARELIB/Applications
export FS_IMGS=$CROSSLAYER_FW/full_system_images

echo "CrossLayerFW: $CROSSLAYER_FW"
