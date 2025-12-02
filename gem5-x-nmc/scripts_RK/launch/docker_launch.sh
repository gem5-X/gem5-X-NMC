 #!/bin/bash

kernel_name=$1
echo "$kernel_name launching on gem5"

cd /CrossLayerNMC/gem5-x-nmc/
#singularity instance start /home/kodra/srv11.sif "$kernel_name"

output_dir="/CrossLayerNMC/gem5-x-nmc/gem5_outputs/$kernel_name"

mkdir "$output_dir"
ln -s /CrossLayerNMC/gem5-x-nmc/gem5_outputs/cpt.6553149866000 /CrossLayerNMC/gem5-x-nmc/gem5_outputs/$kernel_name/

# Uncomment in case gem5 needs to be rebuilt
# singularity exec instance://"$kernel_name" 
scons build/ARM/gem5.fast -j64

#singularity exec instance://"$kernel_name" 
./build/ARM/gem5.fast \
--remote-gdb-port=0 \
-d /CrossLayerNMC/gem5-x-nmc/gem5_outputs/"$kernel_name" \
--stats-file="${kernel_name}_stats.txt" \
--dump-config="${kernel_name}_config.ini" \
configs/example/fs.py \
--cpu-clock=2GHz \
--kernel=vmlinux_wa \
--machine-type=VExpress_GEM5_V1 \
--dtb-file=/CrossLayerNMC/gem5-x-nmc/system/arm/dt/armv8_gem5_v1_1cpu.dtb \
-n 1 \
--disk-image=test_spm.img \
--caches \
--l2cache \
--l1i_size=32kB \
--l1d_size=32kB \
--l2_size=1MB \
--l2_assoc=2 \
--mem-type=DDR4_2400_8x8 \
--mem-ranks=4 \
--mem-size=4GB \
--sys-clock=1600MHz \
--cpu-type=MinorCPU \
--workload-automation-vio=/CrossLayerNMC/gem5-x-nmc/ \
--nmc \
--nmc_mem_type=Ramulator \
--ramulator-config=/CrossLayerNMC/gem5-x-nmc/ext/ramulator/Ramulator/configs/HBM_AB-config.cfg \
--nmc_mem_size=16GB \
-r 1

#singularity instance stop "$kernel_name"