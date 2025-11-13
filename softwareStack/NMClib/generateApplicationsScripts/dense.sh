#!/bin/bash


# dense_args_list=(
#     # Layer 72: Dense Type	| Named dense1	| Input: [2048]		| Output: [1000]		| Weights: [2048, 1000]
#     "-D M=2048 -D N=1000 -o dense1"
#     # Layer 72: Dense Type	| Named dense1	| Input: [1]		| Output: [1000]		| Weights: [2048, 1000] conv layers CO=1
#     "-D M=1 -D N=1000 -o dense1_CO1" 
#     # Layer 72: Dense Type	| Named dense1	| Input: [1]		| Output: [1000]		| Weights: [2048, 1000] dont know if it makes sense but to have it
#     "-D M=2048 -D N=1 -o dense1_CI1" 
# )
echo "Default"


echo "Layer 72 dense1"
echo "Default dimensions: M=2048 N=1000"


g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1
sshpass -p "vlMoDwsg9d" scp dense1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense


sed -i "s/#define M   2048/#define M   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_M1
sshpass -p "vlMoDwsg9d" scp dense1_M1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense

sed -i "s/#define M   1/#define M   2048/g" main.cpp  
sed -i "s/#define N   1000/#define N   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_N1
sshpass -p "vlMoDwsg9d" scp dense1_N1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense

sed -i "s/#define N   1/#define N   1000/g" main.cpp  

echo "DDR4"
sed -i "s/#define DRAM 0/#define DRAM 1/g" defs.h  
echo "Layer 72 dense1"
echo "Default dimensions: M=2048 N=1000"


g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_DDR4
sshpass -p "vlMoDwsg9d" scp dense1_DDR4 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_DDR4


sed -i "s/#define M   2048/#define M   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_DDR4_M1
sshpass -p "vlMoDwsg9d" scp dense1_DDR4_M1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_DDR4

sed -i "s/#define M   1/#define M   2048/g" main.cpp  
sed -i "s/#define N   1000/#define N   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_DDR4_N1
sshpass -p "vlMoDwsg9d" scp dense1_DDR4_N1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_DDR4

sed -i "s/#define N   1/#define N   1000/g" main.cpp  

echo "GDDR5"
sed -i "s/#define DRAM 1/#define DRAM 2/g" defs.h  
echo "Layer 72 dense1"
echo "Default dimensions: M=2048 N=1000"


g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_GDDR5
sshpass -p "vlMoDwsg9d" scp dense1_GDDR5 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_GDDR5


sed -i "s/#define M   2048/#define M   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_GDDR5_M1
sshpass -p "vlMoDwsg9d" scp dense1_GDDR5_M1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_GDDR5

sed -i "s/#define M   1/#define M   2048/g" main.cpp  
sed -i "s/#define N   1000/#define N   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_GDDR5_N1
sshpass -p "vlMoDwsg9d" scp dense1_GDDR5_N1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_GDDR5

sed -i "s/#define N   1/#define N   1000/g" main.cpp 


echo "LPDDR4"
sed -i "s/#define DRAM 2/#define DRAM 3/g" defs.h  
echo "Layer 72 dense1"
echo "Default dimensions: M=2048 N=1000"


g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_LPDDR4
sshpass -p "vlMoDwsg9d" scp dense1_LPDDR4 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_LPDDR4


sed -i "s/#define M   2048/#define M   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_LPDDR4_M1
sshpass -p "vlMoDwsg9d" scp dense1_LPDDR4_M1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_LPDDR4

sed -i "s/#define M   1/#define M   2048/g" main.cpp  
sed -i "s/#define N   1000/#define N   1/g" main.cpp  
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DMVM -o dense1_LPDDR4_N1
sshpass -p "vlMoDwsg9d" scp dense1_LPDDR4_N1 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDRes_Dense_LPDDR4

sed -i "s/#define N   1/#define N   1000/g" main.cpp  

sed -i "s/#define DRAM 3/#define DRAM 0/g" defs.h  