#!/bin/bash

declare -a Interconnect=("tp100Gbps" "ucie")
clusters=$2
cores=$3
if [ "${cores}" -gt 9 ]
then
    declare -a numCore=("00" "01" "02" "03" "04" "05" "06" "07" "08" "09" "10" "11" "12" "13" "14" "15")
else
    declare -a numCore=("0" "1" "2" "3" "4" "5" "6" "7" "8" "9")
fi
L2off=$((5+cores*6))
L3off=$((5+cores*6+clusters*1))

for ic in ${Interconnect[@]}
do
    for t in 310 320 330 340 350 360
    do
        # echo -n "Interconnect,Clusters,Cores,Frequency (GHz),Runtime (ps),CPU Peak dynamic (W),CPU Leakage (W),CPU Leakage with Power gating,L2 Peak dynamic (W),L2 Leakage (W),L2 Leakage with Power gating (W),L3 Peak dynamic (W),L3 Leakage (W),L3 Leakage with Power gating (W)," > $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        echo -n "Interconnect,Clusters,Cores,Frequency (GHz),Runtime (ps),CPU Peak dynamic (W),CPU Leakage (W),CPU Leakage with Power gating," > $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        for (( i=0; i<${clusters}; i++ ))
        do
            echo -n "L2_${i} accesses,L3_${i} accesses," >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        done
        for (( i=0; i<${cores}; i++ ))
        do
            # echo -n "CPU${i} Sim. cycles,CPU${i} Idle cycles,CPU${i} ON cycles,CPU${i} CLK_GATED cycles,L2_${i} WL transmissions,L2_${i} WL collisions,L2_${i} WL latency (clk),L2_${i} WL bytes" >> $1_${ic}_${t}k.csv
            echo -n "CPU${i} Sim. cycles,CPU${i} Idle cycles,CPU${i} ON (ps),CPU${i} CLK_GATED (ps),CPU${i} OFF (ps)," >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        done
        # truncate -s -1 $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        echo -n "Bytes transmitted" >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        echo "" >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv

        for f in 1 2 3
        do
            # Interconnect, clusters, cores and frequency
            echo -n "${ic},${clusters},${cores},${f}," >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # Runtime
            grep -m 1 -e sim_ticks ${f}ghz_${ic}_stats.txt > aux.txt
            sed -i 's/sim_ticks//g' aux.txt
            sed -i 's/#.*//g' aux.txt
            tr -d ' ' < aux.txt > aux2.txt
            truncate -s -1 aux2.txt
            echo -n "," >> aux2.txt
            cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # CPU Peak dynamic
            grep -m 5 "Peak Dynamic =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            sed -i 's/Peak Dynamic =//g' aux.txt
            sed -i 's/W//g' aux.txt
            tr -d ' ' < aux.txt > aux2.txt
            truncate -s -1 aux2.txt
            echo -n "," >> aux2.txt
            cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # CPU Leakage
            grep -m5 "Subthreshold Leakage =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            sed -i 's/Subthreshold Leakage =//g' aux.txt
            sed -i 's/W//g' aux.txt
            tr -d ' ' < aux.txt > aux2.txt
            truncate -s -1 aux2.txt
            echo -n "," >> aux2.txt
            cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # CPU Leakage with power gating
            grep -m5 "Subthreshold Leakage with power gating =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            sed -i 's/Subthreshold Leakage with power gating =//g' aux.txt
            sed -i 's/W//g' aux.txt
            tr -d ' ' < aux.txt > aux2.txt
            truncate -s -1 aux2.txt
            echo -n "," >> aux2.txt
            cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            if [ "${clusters}" -lt 2 ]
            then
                # L2 accesses
                grep -m 1 -e system.l2.overall_accesses::total ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.l2.overall_accesses::total//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # L3 accesses,
                grep -m 1 -e system.l3.overall_accesses::total ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.l3.overall_accesses::total//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            else
                for (( i=0; i<${clusters}; i++ ))
                do
                    # L2 accesses
                    grep -m 1 -e system.l2${i}.overall_accesses::total ${f}ghz_${ic}_stats.txt > aux.txt
                    sed -i 's/system.l2'"${i}"'.overall_accesses::total//g' aux.txt
                    sed -i 's/#.*//g' aux.txt
                    tr -d ' ' < aux.txt > aux2.txt
                    truncate -s -1 aux2.txt
                    echo -n "," >> aux2.txt
                    cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                    # L3 accesses,
                    grep -m 1 -e system.l3${i}.overall_accesses::total ${f}ghz_${ic}_stats.txt > aux.txt
                    sed -i 's/system.l3'"${i}"'.overall_accesses::total//g' aux.txt
                    sed -i 's/#.*//g' aux.txt
                    tr -d ' ' < aux.txt > aux2.txt
                    truncate -s -1 aux2.txt
                    echo -n "," >> aux2.txt
                    cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                done
            fi
            # # L2 Peak dynamic
            # grep -m${L2off} "Peak Dynamic =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Peak Dynamic =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # # L2 Leakage
            # grep -m${L2off} "Subthreshold Leakage =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Subthreshold Leakage =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # # L2 Leakage with power gating
            # grep -m${L2off} "Subthreshold Leakage with power gating =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Subthreshold Leakage with power gating =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # # L3 Peak dynamic
            # grep -m${L3off} "Peak Dynamic =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Peak Dynamic =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # # L3 Leakage
            # grep -m${L3off} "Subthreshold Leakage =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Subthreshold Leakage =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            # # L3 Leakage with power gating
            # grep -m${L3off} "Subthreshold Leakage with power gating =" ${f}ghz_${ic}_power_${t}k.txt | tail -n1 > aux.txt
            # sed -i 's/Subthreshold Leakage with power gating =//g' aux.txt
            # sed -i 's/W//g' aux.txt
            # tr -d ' ' < aux.txt > aux2.txt
            # truncate -s -1 aux2.txt
            # echo -n "," >> aux2.txt
            # cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            for (( i=0; i<${cores}; i++ ))
            do
                # CPU Sim. cycles
                grep -m 1 -e system.switch_cpus${numCore[i]}.numCycles ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.switch_cpus'"${numCore[i]}"'.numCycles//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # CPU Idle cycles
                grep -m 1 -e system.switch_cpus${numCore[i]}.idleCycles ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.switch_cpus'"${numCore[i]}"'.idleCycles//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # CPU ON cycles
                grep -m 1 -e system.switch_cpus${numCore[i]}.pwrStateResidencyTicks::ON ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.switch_cpus'"${numCore[i]}"'.pwrStateResidencyTicks::ON//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # CPU CLK_GATED cycles
                grep -m 1 -e system.switch_cpus${numCore[i]}.pwrStateResidencyTicks::CLK_GATED ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.switch_cpus'"${numCore[i]}"'.pwrStateResidencyTicks::CLK_GATED//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # CPU OFF cycles
                grep -m 1 -e system.switch_cpus${numCore[i]}.pwrStateResidencyTicks::OFF ${f}ghz_${ic}_stats.txt > aux.txt
                sed -i 's/system.switch_cpus'"${numCore[i]}"'.pwrStateResidencyTicks::OFF//g' aux.txt
                sed -i 's/#.*//g' aux.txt
                tr -d ' ' < aux.txt > aux2.txt
                truncate -s -1 aux2.txt
                echo -n "," >> aux2.txt
                cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
                # L2 WL transmissions
                # L2 WL collisions
                # L2 WL latency
                # L2 WL bytes
            done
            # Bytes transmitted
            grep -m 1 -e system.membus.bytes_transmitted::total ${f}ghz_${ic}_stats.txt > aux.txt
            sed -i 's/system.membus.bytes_transmitted::total//g' aux.txt
            sed -i 's/#.*//g' aux.txt
            tr -d ' ' < aux.txt > aux2.txt
            truncate -s -1 aux2.txt
            echo -n "," >> aux2.txt
            cat aux2.txt >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv

            truncate -s -1 $1_${clusters}cl${cores}c_${ic}_${t}k.csv
            echo "" >> $1_${clusters}cl${cores}c_${ic}_${t}k.csv
        done
    done
done

rm aux.txt aux2.txt
