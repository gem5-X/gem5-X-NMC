 #!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: sh $0 <container_path_of_checkpoint>"
    echo "Example: sh $0 /CrossLayerNMC/gem5-x-nmc/gem5_outputs/cpt.6553149866000"
    exit 1
fi

sed -i "s|/CrossLayerNMC/gem5-x-nmc/gem5_outputs/cpt.number|$1|g" /CrossLayerNMC/gem5-x-nmc/scripts_RK/launch/docker_launch.sh