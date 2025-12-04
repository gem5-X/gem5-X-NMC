#!/bin/bash
# Usage ./scripts/setUpFramework.sh <image_name>

if [ -z "$1" ]; then
    echo "Usage: source ${BASH_SOURCE[0]} <image_name>"
    return 1 2>/dev/null || exit 1
fi

# Get the full  system images from gem5 online
# clarify that the path to them in gem5-x is hardcoded so that it is compatible with the docker image
# Tell them to get from the gem5-x 

source scripts/export_paths.sh


# Clone Eigen (Branch we used)
if [ ! -f "$SOFTWARELIB/eigen" ]; then
    echo "./scripts/install_eigen.sh"
    # ./scripts/install_eigen.sh
fi

# Build docker image
docker build -t $1 $CROSSLAYER_FW

