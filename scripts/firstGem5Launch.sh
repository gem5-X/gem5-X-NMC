#!/bin/bash
if [ -z "$2" ]; then
    echo "Usage: $0 <container_name> <image_name>"
    exit 1
fi

WORKS for GEM5 (04.12.2025) with actual FS_IMGS path
# Build and launch gem5 for the first time to create a checkpoint 
docker run -it --rm --name $1 \
-v $FS_IMGS:/gem5-X-NMC/full_system_images \
$2


# Give verbal instructions