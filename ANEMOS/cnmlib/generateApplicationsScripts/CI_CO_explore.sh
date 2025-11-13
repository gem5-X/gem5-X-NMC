
#!/bin/bash

echo "CIxCO exploration"

echo "Layer 35. res4a_branch2c"
echo "Default dimensions: HI=14 WI=14 CI=256 K=1 CO=1024 PADDING=0 STRIDE=1"

echo "CI exploration"
for CI in 1 2 4 8 16 32 64 128 256 ; do
    sed -i "s/#define CI  256/#define CI  $CI/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res4a_branch2c_"$CI"x1024
    sshpass -p "vlMoDwsg9d" scp res4a_branch2c_"$CI"x1024 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CI  $CI/#define CI  256/g" main.cpp  
done

echo "CO exploration"
for CO in 1 2 4 8 16 32 64 128 256 512 1024 ; do
    sed -i "s/#define CO  1024/#define CO  $CO/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res4a_branch2c_256x"$CO"
    sshpass -p "vlMoDwsg9d" scp res4a_branch2c_256x"$CO" kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CO  $CO/#define CO  1024/g" main.cpp  
done

echo "Layer 37. res4b_branch2a"
echo "Default dimensions: HI=14 WI=14 CI=1024 K=1 CO=256 PADDING=0 STRIDE=1"

sed -i "s/#define CI  256/#define CI  1024/g" main.cpp  
sed -i "s/#define CO  1024/#define CO 256/g" main.cpp  

echo "CI exploration"
for CI in 1 2 4 8 16 32 64 128 256 512 1024 ; do
    sed -i "s/#define CI  1024/#define CI  $CI/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res4b_branch2a_"$CI"x256
    sshpass -p "vlMoDwsg9d" scp res4b_branch2a_"$CI"x256 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CI  $CI/#define CI  1024/g" main.cpp  
done

echo "CO exploration"
for CO in 1 2 4 8 16 32 64 128 256; do
    sed -i "s/#define CO  256/#define CO  $CO/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res4b_branch2a_1024x"$CO"
    sshpass -p "vlMoDwsg9d" scp res4b_branch2a_1024x"$CO" kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CO  $CO/#define CO  256/g" main.cpp  
done

echo "Layer 59. res5a_branch2b"
echo "Default dimensions: HI=7 WI=7 CI=512 K=3 CO=512 PADDING=1 STRIDE=1"

sed -i "s/#define CI  1024/#define CI  512/g" main.cpp  
sed -i "s/#define CO  256/#define CO 512/g" main.cpp 
sed -i "s/#define HI  14/#define HI  7/g" main.cpp  
sed -i "s/#define WI  14/#define WI  7/g" main.cpp   
sed -i "s/#define PADDING     0/#define PADDING     1/g" main.cpp  

echo "CI exploration"
for CI in 1 2 4 8 16 32 64 128 256 512 ; do
    sed -i "s/#define CI  512/#define CI  $CI/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res5a_branch2b_"$CI"x512
    sshpass -p "vlMoDwsg9d" scp res5a_branch2b_"$CI"x512 kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CI  $CI/#define CI  512/g" main.cpp  
done

echo "CO exploration"
for CO in 1 2 4 8 16 32 64 128 256 512 ; do
    sed -i "s/#define CO  512/#define CO  $CO/g" main.cpp  
    g++ -std=c++11 -O3 main.cpp -I /cnm-framework/pim-cores/cnmlib/include -L /cnm-framework/pim-cores/cnmlib/util/m5 -lm5 -lpthread -Wall -DCONV -o res5a_branch2b_512x"$CO"
    sshpass -p "vlMoDwsg9d" scp res5a_branch2b_512x"$CO" kodra@eslsrv11.epfl.ch:/home/kodra/shares/local/scrap/gem5-x-cnm/applications/SSDResNet_CIxCO
    sed -i "s/#define CO  $CO/#define CO  512/g" main.cpp  
done