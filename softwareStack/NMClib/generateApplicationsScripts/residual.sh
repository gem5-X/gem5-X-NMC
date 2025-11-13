#!/bin/bash

res_args_list=(
    # Layer 6: Residual Type	| Named res2a_end	| Input: [256, 56, 56]	| Output: [256, 56, 56]	| Residual: [256, 56, 56]
    "-D NV=14336 -D ND=56 -o debug_res2a_end_DDR4_extraLoops_Rlim"
    # # # Layer 19: Residual Type	| Named res3a_end	| Input: [512, 28, 28]	| Output: [512, 28, 28]	| Residual: [512, 28, 28]
    # "-D NV=14336 -D ND=28 -o debug_res3a_end_DDR4"
    # # # Layer 36: Residual Type	| Named res4a_end	| Input: [1024, 14, 14]	| Output: [1024, 14, 14]	| Residual: [1024, 14, 14]
    # "-D NV=14336 -D ND=14 -o debug_res4a_end_DDR4"
    # Layer 61: Residual Type	| Named res5a_end	| Input: [2048, 7, 7]	| Output: [2048, 7, 7]	| Residual: [2048, 7, 7]
    # "-D NV=14336 -D ND=7 -o debug_res5a_end_DDR4"
    # # Layer 6: Residual Type	| Named res2a_end	| Input: [1, 56, 56]	| Output: [1, 56, 56]	| Residual: [1, 56, 56]
    # "-D NV=56 -D ND=56 -o debug_res2a_end_DDR4_C1"
    # # # Layer 19: Residual Type	| Named res3a_end	| Input: [1, 28, 28]	| Output: [1, 28, 28]	| Residual: [1, 28, 28]
    # "-D NV=28 -D ND=28 -o debug_res3a_end_DDR4_C1"
    # # # Layer 36: Residual Type	| Named res4a_end	| Input: [1, 14, 14]	| Output: [1, 14, 14]	| Residual: [1, 14, 14]
    # "-D NV=14 -D ND=14 -o debug_res4a_end_DDR4_C1"
    # # Layer 61: Residual Type	| Named res5a_end	| Input: [1, 7, 7]	| Output: [1, 7, 7]	| Residual: [1, 7, 7]
    # "-D NV=7 -D ND=7 -o debug_res5a_end_DDR4_C1"

    # "-D NV=29 -D ND=30 -o smallRes"
    # "-D NV=56 -D ND=14336 -o alternateNV_ND"
    # "-D NV=719 -D ND=911 -o noPeeling"

)


for res_args in "${res_args_list[@]}"; do
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DVA $res_args
    sshpass -p "vlMoDwsg9d" scp $(echo "$res_args" | awk -F'-o ' '{print $2}') kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/debug_SSDResNet34_DDR4/cnm_res
done
