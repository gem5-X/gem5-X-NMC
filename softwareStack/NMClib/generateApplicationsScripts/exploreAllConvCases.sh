
#!/bin/bash

# echo "All Convolution Cases Testing"

# echo "===================================R-limited Kernels==================================="

echo "1a. ext_loops = 0, relu = 0, ext_peeling = 0"

sed -i "s/#define CI  1/#define CI  8/g" main.cpp 
sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_1a
sshpass -p "vlMoDwsg9d" scp  limR_1a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
sed -i "s/#define CI  8/#define CI  1/g" main.cpp 
sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

echo "1b. ext_loops = 0, relu = 1, ext_peeling = 0"

sed -i "s/#define CI  1/#define CI  8/g" main.cpp 
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_1b
sshpass -p "vlMoDwsg9d" scp  limR_1b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
sed -i "s/#define CI  8/#define CI  1/g" main.cpp 

echo "2 . ext_loops > 0, relu = 0, ext_peeling = 0"

sed -i "s/#define CI  1/#define CI  16/g" main.cpp 
sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_2
sshpass -p "vlMoDwsg9d" scp  limR_2 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
sed -i "s/#define CI  16/#define CI  1/g" main.cpp 
sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "3a. ext_loops < 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  4/g" main.cpp 
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_3a
# sshpass -p "vlMoDwsg9d" scp  limR_3a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  4/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "3b. ext_loops < 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  4/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_3b
# sshpass -p "vlMoDwsg9d" scp  limR_3b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  4/#define CI  1/g" main.cpp 

echo "4 . ext_loops > 0, relu = 1, ext_peeling = 0"

sed -i "s/#define CI  1/#define CI  16/g" main.cpp 
g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_4
sshpass -p "vlMoDwsg9d" scp  limR_4 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
sed -i "s/#define CI  16/#define CI  1/g" main.cpp 

# echo "5a. ext_loops > 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  10/g" main.cpp
# sed -i "s/#define K   1/#define K   3/g" main.cpp  
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_5a
# sshpass -p "vlMoDwsg9d" scp  limR_5a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  10/#define CI  1/g" main.cpp 
# sed -i "s/#define K   3/#define K   1/g" main.cpp  
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "5b. ext_loops > 0, relu = 1, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  10/g" main.cpp
# sed -i "s/#define K   1/#define K   3/g" main.cpp  
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_5b
# sshpass -p "vlMoDwsg9d" scp  limR_5b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  10/#define CI  1/g" main.cpp 
# sed -i "s/#define K   3/#define K   1/g" main.cpp  

# echo "6a. ext_loops = 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  4/g" main.cpp
# sed -i "s/#define K   1/#define K   3/g" main.cpp  
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_6a
# sshpass -p "vlMoDwsg9d" scp  limR_6a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  4/#define CI  1/g" main.cpp 
# sed -i "s/#define K   3/#define K   1/g" main.cpp  
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "6b. ext_loops = 0, relu = 1, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  4/g" main.cpp
# sed -i "s/#define K   1/#define K   3/g" main.cpp  
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limR_6b
# sshpass -p "vlMoDwsg9d" scp  limR_6b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  4/#define CI  1/g" main.cpp 
# sed -i "s/#define K   3/#define K   1/g" main.cpp  

# echo "===================================C-limited Kernels==================================="

# sed -i "s/#define CRF_ENTRIES     32/#define CRF_ENTRIES     8/g" defs.h

# echo "1a. ext_loops = 0, relu = 0, ext_peeling = 0"

# sed -i "s/#define CI  1/#define CI  4/g" main.cpp 
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_1a
# sshpass -p "vlMoDwsg9d" scp  limC_1a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  4/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "1b. ext_loops = 0, relu = 1, ext_peeling = 0"

# sed -i "s/#define CI  1/#define CI  2/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_1b
# sshpass -p "vlMoDwsg9d" scp  limC_1b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  2/#define CI  1/g" main.cpp 

# echo "2 . ext_loops > 0, relu = 0, ext_peeling = 0"

# sed -i "s/#define CI  1/#define CI  8/g" main.cpp 
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_2
# sshpass -p "vlMoDwsg9d" scp  limC_2 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  8/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "3a. ext_loops < 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  3/g" main.cpp 
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_3a
# sshpass -p "vlMoDwsg9d" scp  limC_3a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  3/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "3b. ext_loops < 0, relu = 0, ext_peeling > 0"

# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_3b
# sshpass -p "vlMoDwsg9d" scp  limC_3b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases

# echo "4 . ext_loops > 0, relu = 1, ext_peeling = 0"

# sed -i "s/#define CI  1/#define CI  8/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_4
# sshpass -p "vlMoDwsg9d" scp  limC_4 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  8/#define CI  1/g" main.cpp 

# echo "5a. ext_loops > 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  10/g" main.cpp
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_5a
# sshpass -p "vlMoDwsg9d" scp  limC_5a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  10/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "5b. ext_loops > 0, relu = 1, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  5/g" main.cpp
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_5b
# sshpass -p "vlMoDwsg9d" scp  limC_5b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  5/#define CI  1/g" main.cpp 

# echo "6a. ext_loops = 0, relu = 0, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  5/g" main.cpp
# sed -i "s/#define RELU        1/#define RELU        0/g" main.cpp 
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_6a
# sshpass -p "vlMoDwsg9d" scp  limC_6a kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  5/#define CI  1/g" main.cpp 
# sed -i "s/#define RELU        0/#define RELU        1/g" main.cpp 

# echo "6a. ext_loops = 0, relu = 1, ext_peeling > 0"

# sed -i "s/#define CI  1/#define CI  3/g" main.cpp
# g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o limC_6b
# sshpass -p "vlMoDwsg9d" scp  limC_6b kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/CnMConvTestCases
# sed -i "s/#define CI  3/#define CI  1/g" main.cpp 

# sed -i "s/#define CRF_ENTRIES     8/#define CRF_ENTRIES     32/g" defs.h