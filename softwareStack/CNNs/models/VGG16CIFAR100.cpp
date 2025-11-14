/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Implementation of custom single-core VGG16 CNN based on VGG16 with
 * CIFAR100 provided by Flavio Ponzina.
 *
 */

#include <iostream>

#include "tinytensorlib.hh"
#include "model.hh"
#include "gem5/m5ops.h"

using namespace std;


int
main(int argc, char * argv[])
{
#ifdef CNM
    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* conv_kernel;
    Kernel* mvm_kernel;
    Kernel* va_kernel;
    uint channel = 0;
#endif
    // Initialize buffers and other vars.
    //int sys_info = 0;

    generateSingleOutputDataStructure(conv1);
    printLayerInfo(&conv1);
    
    connectLayers(conv2, conv1);
    connectLayers(pool1, conv2);

    connectLayers(conv3, pool1);
    connectLayers(conv4, conv3);
    connectLayers(pool2, conv4);

    connectLayers(conv5, pool2);
    connectLayers(conv6, conv5);
    connectLayers(conv7, conv6);
    connectLayers(pool3, conv7);

    connectLayers(conv8, pool3);
    connectLayers(conv9, conv8);
    connectLayers(conv10, conv9);
    connectLayers(pool4, conv10);

    connectLayers(conv11, pool4);
    connectLayers(conv12, conv11);
    connectLayers(conv13, conv12);
    connectLayers(pool5, conv13);

    connectLayers(flatten1, pool5);
    connectLayers(dense1, flatten1);

    dense2.input = dense1.output;
    printLayerInfo(&dense2);

    // Do inference.
    //sys_info += system("m5 resetstats");
    for (int inf = 0; inf < T_x; inf++)
    {  
        
        //cout << "Inference " << inf << endl;
        m5_reset_stats(0,0);
        doLayer(conv1, inf, 0);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(conv2);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(pool1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(conv3);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(conv4);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(pool2);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(conv5);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(conv6);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(conv7);
#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(pool3);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(conv8);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(conv9);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(conv10);
#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(pool4);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(conv11);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(conv12);
        doLayer(conv13);
#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(pool5);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(flatten1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(dense1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(dense2, 0, inf);
//#ifdef STATS
        m5_dump_stats(0,0);
//#endif

#ifdef CNM
        m5_reset_stats(0,0);
        cnmMemoryMap(cnmElements);

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doConv_CnM(conv1, cnmElements, conv_kernel, channel);
        CnM2Eigen(conv1.CNM_res, conv1.CnM2Eigen_3Doutput);
        Normalization(conv1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv1.CnM2Eigen_3Doutput, conv2.CNM_input);
        doConv_CnM(conv2, cnmElements, conv_kernel, channel);
        Normalization(conv2.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv2 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        pool1.CnM2Eigen_3Dinput = conv2.CnM2Eigen_3Doutput; 
        Pooling(&pool1, pool1.CnM2Eigen_3Dinput, pool1.CnM2Eigen_3Doutput, pool1.pool_type);
        //std::cout << "pool1 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool1.CnM2Eigen_3Doutput, conv3.CNM_input);
        doConv_CnM(conv3, cnmElements, conv_kernel, channel);
        CnM2Eigen(conv3.CNM_res, conv3.CnM2Eigen_3Doutput);
        Normalization(conv3.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv3 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv3.CnM2Eigen_3Doutput, conv4.CNM_input);
        doConv_CnM(conv4, cnmElements, conv_kernel, channel);
        Normalization(conv4.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv4 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        pool2.CnM2Eigen_3Dinput = conv3.CnM2Eigen_3Doutput; 
        Pooling(&pool2, pool2.CnM2Eigen_3Dinput, pool2.CnM2Eigen_3Doutput, pool2.pool_type);
        //std::cout << "pool2 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool2.CnM2Eigen_3Doutput, conv5.CNM_input);
        doConv_CnM(conv5, cnmElements, conv_kernel, channel);
        CnM2Eigen(conv5.CNM_res, conv5.CnM2Eigen_3Doutput);
        Normalization(conv5.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv5 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv5.CnM2Eigen_3Doutput, conv6.CNM_input);
        doConv_CnM(conv6, cnmElements, conv_kernel, channel);
        Normalization(conv6.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv6 Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif

        initialize_3D_Matrix(conv6.CnM2Eigen_3Doutput, conv7.CNM_input);
        doConv_CnM(conv7, cnmElements, conv_kernel, channel);
        Normalization(conv7.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv7 Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

        pool3.CnM2Eigen_3Dinput = conv7.CnM2Eigen_3Doutput; 
        Pooling(&pool3, pool3.CnM2Eigen_3Dinput, pool3.CnM2Eigen_3Doutput, pool3.pool_type);
        //std::cout << "pool3 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool3.CnM2Eigen_3Doutput, conv8.CNM_input);
        doConv_CnM(conv8, cnmElements, conv_kernel, channel);
        CnM2Eigen(conv8.CNM_res, conv8.CnM2Eigen_3Doutput);
        Normalization(conv8.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv8 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv8.CnM2Eigen_3Doutput, conv9.CNM_input);
        doConv_CnM(conv9, cnmElements, conv_kernel, channel);
        Normalization(conv9.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv9 Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif

        initialize_3D_Matrix(conv9.CnM2Eigen_3Doutput, conv10.CNM_input);
        doConv_CnM(conv10, cnmElements, conv_kernel, channel);
        Normalization(conv10.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv10 Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

        pool4.CnM2Eigen_3Dinput = conv10.CnM2Eigen_3Doutput; 
        Pooling(&pool4, pool4.CnM2Eigen_3Dinput, pool4.CnM2Eigen_3Doutput, pool4.pool_type);
        //std::cout << "pool4 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool4.CnM2Eigen_3Doutput, conv11.CNM_input);
        doConv_CnM(conv11, cnmElements, conv_kernel, channel);
        CnM2Eigen(conv11.CNM_res, conv11.CnM2Eigen_3Doutput);
        Normalization(conv11.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv11 Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif

        initialize_3D_Matrix(conv11.CnM2Eigen_3Doutput, conv12.CNM_input);
        doConv_CnM(conv12, cnmElements, conv_kernel, channel);
        Normalization(conv12.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv12 Norm" << std::endl;

        initialize_3D_Matrix(conv12.CnM2Eigen_3Doutput, conv13.CNM_input);
        doConv_CnM(conv13, cnmElements, conv_kernel, channel);
        Normalization(conv13.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv13 Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

        pool5.CnM2Eigen_3Dinput = conv13.CnM2Eigen_3Doutput; 
        Pooling(&pool5, pool5.CnM2Eigen_3Dinput, pool5.CnM2Eigen_3Doutput, pool5.pool_type);
        //std::cout << "pool5 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif


        flatten1.CnM2Eigen_3Dinput = pool5.CnM2Eigen_3Doutput;
        Flatten(flatten1.CnM2Eigen_3Dinput, flatten1.CnM2Eigen_1Doutput);
        //std::cout << "flatten1 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_vector(flatten1.CnM2Eigen_1Doutput, dense1.CNM_v1);
        doFC_CnM(dense1, cnmElements, mvm_kernel, channel);
        //std::cout << "dense 1" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        dense2.CNM_v1 = dense1.CNM_res;
        doFC_CnM(dense2, cnmElements, mvm_kernel, channel);
        //std::cout << "dense 2" << std::endl;
// #ifdef STATS
//         m5_dump_reset_stats(0,0);
// #endif

#endif
    }

    // Finish and clean up.
    m5_dump_stats(0,0);
    m5_exit(0);

    delete[] conv1.input;
    delete[] conv2.input;
    delete[] pool1.input;

    delete[] conv3.input;
    delete[] conv4.input;
    delete[] pool2.input;

    delete[] conv5.input;
    delete[] conv6.input;
    delete[] conv7.input;
    delete[] pool3.input;

    delete[] conv8.input;
    delete[] conv9.input;
    delete[] conv10.input;
    delete[] pool4.input;

    delete[] conv11.input;
    delete[] conv12.input;
    delete[] conv13.input;
    delete[] pool5.input;

    delete[] flatten1.input;
    delete[] dense1.input;
    delete[] dense2.input;
    delete[] dense2.output;

    return 0;
}
