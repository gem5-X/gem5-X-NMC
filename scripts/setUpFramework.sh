#!/bin/bash
# Usage ./scripts/setUpFramework.sh <image_name>

if [ -z "$1" ]; then
    echo "Usage: source ${BASH_SOURCE[0]} <image_name>"
    return 1 2>/dev/null || exit 1
fi

source scripts/export_paths.sh

# Clone Eigen (Branch we used)
if [ ! -f "$SOFTWARELIB/eigen" ]; then
    echo "./scripts/install_eigen.sh"
    ./scripts/install_eigen.sh
fi

# Build docker image
docker build -t $1 $CROSSLAYER_FW

