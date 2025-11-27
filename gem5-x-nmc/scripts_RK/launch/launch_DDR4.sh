 #!/bin/bash

kernel_name=$1
echo "$kernel_name launching on gem5"

cd /home/kodra/CrossLayerNMC/gem5-x-nmc/
# apptainer instance start /home/kodra/srv11.sif "$kernel_name"

# output_dir="/CrossLayerNMC/gem5-x-nmc/$kernel_name"
output_dir=gem5_outputs

# mkdir "$output_dir"
# ln -s /home/kodra/shares/local/scrap/gem5-x-cnm/gem5_outputs/firstLaunchNewPimcores/cpt.7494845714500 /home/kodra/shares/local/scrap/gem5-x-cnm/gem5_outputs/debug_SSDResNet34_DDR4/$kernel_name/

# Uncomment in case gem5 needs to be rebuilt
# singularity exec instance://"$kernel_name" scons build/ARM/gem5.fast -j64

# singularity exec instance://"$kernel_name" 
./build/ARM/gem5.fast \
--remote-gdb-port=0 \
-d /CrossLayerNMC/gem5-x-nmc/gem5_outputs \
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
--workload-automation-vio=/CrossLayerNMC/gem5-x-nmc \
--nmc \
--nmc_mem_type=Ramulator \
--ramulator-config=/CrossLayerNMC/gem5-x-nmc/ext/ramulator/Ramulator/configs/DDR4_AB-config.cfg \
--nmc_mem_size=16GB \
-r 1

# singularity instance stop "$kernel_name"