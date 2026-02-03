 #!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: sh $0 <name_of_gem5_build>"
    echo "Example: sh $0 HBM"
    exit 1
fi

# echo "SCRIPT ARG = '$1'"
# sed -i "s|nmc-cores|nmc-cores-$1|g" /gem5-X-NMC/gem5-x-nmc/src/mem/nmccores.cc

kernel_name=first_checkpoint
echo "$kernel_name launching on gem5"

cd /gem5-X-NMC/gem5-x-nmc/

output_dir="/gem5-X-NMC/gem5-x-nmc/gem5_outputs/$kernel_name"

mkdir "$output_dir"

./build/ARM/gem5_$1.fast \
--remote-gdb-port=0 \
-d /gem5-X-NMC/gem5-x-nmc/gem5_outputs/"$kernel_name" \
--stats-file="${kernel_name}_stats.txt" \
--dump-config="${kernel_name}_config.ini" \
configs/example/fs.py \
--cpu-clock=2GHz \
--kernel=vmlinux_wa \
--machine-type=VExpress_GEM5_V1 \
--dtb-file=/gem5-X-NMC/gem5-x-nmc/system/arm/dt/armv8_gem5_v1_1cpu.dtb \
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
--cpu-type=AtomicSimpleCPU \
--workload-automation-vio=/gem5-X-NMC/softwareStack/Applications \
--nmc \
--nmc_mem_type=Ramulator \
--ramulator-config=/gem5-X-NMC/gem5-x-nmc/ext/ramulator/Ramulator/configs/HBM_AB-config.cfg \
--nmc_mem_size=16GB