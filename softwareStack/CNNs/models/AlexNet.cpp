/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Implementation of a single-core AlexNet CNN with random weights and inputs
 * using Eigen C++ library and Tensor utilities.
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
    
    connectLayers(pool1, conv1);
    connectLayers(conv2, pool1);
    connectLayers(pool2, conv2);
    connectLayers(conv3, pool2);
    connectLayers(conv4, conv3);
    connectLayers(conv5, conv4);
    connectLayers(pool3, conv5);
    connectLayers(flatten1, pool3);
    connectLayers(dense1, flatten1);
    connectLayers(dense2, dense1);

    dense3.input = dense2.output;
    printLayerInfo(&dense3);

    // Do inference.
    //sys_info += system("m5 resetstats");
    for (int inf = 0; inf < T_x; inf++)
    {   

        //cout << "Inference " << inf << endl;
        m5_reset_stats(0,0);
        Convolution2D(&conv1, conv1.input[inf], conv1.weights, conv1.output[0]);
        Activation(conv1.output[0], conv1.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Pooling(&pool1, pool1.input[0], pool1.output[0], pool1.pool_type);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        Convolution2D(&conv2, conv2.input[0], conv2.weights, conv2.output[0]);
        Activation(conv2.output[0], conv2.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Pooling(&pool2, pool2.input[0], pool2.output[0], pool2.pool_type);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        Convolution2D(&conv3, conv3.input[0], conv3.weights, conv3.output[0]);
        Activation(conv3.output[0], conv3.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        Convolution2D(&conv4, conv4.input[0], conv4.weights, conv4.output[0]);
        Activation(conv4.output[0], conv4.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        Convolution2D(&conv5, conv5.input[0], conv5.weights, conv5.output[0]);
        Activation(conv5.output[0], conv5.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Pooling(&pool3, pool3.input[0], pool3.output[0], pool3.pool_type);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Flatten(flatten1.input[0], flatten1.output[0]);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        FullyConnected(&dense1, dense1.input[0], dense1.output[0]);
        Activation(dense1.output[0], dense1.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        FullyConnected(&dense2, dense2.input[0], dense2.output[0]);
        Activation(dense2.output[0], dense2.activation);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        FullyConnected(&dense3, dense3.input[0], dense3.output[inf]);
        Activation(dense3.output[inf], dense3.activation);
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

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        CnM2Eigen(conv1.CNM_res, conv1.CnM2Eigen_3Doutput);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Normalization(conv1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        pool1.CnM2Eigen_3Dinput = conv1.CnM2Eigen_3Doutput; 
        Pooling(&pool1, pool1.CnM2Eigen_3Dinput, pool1.CnM2Eigen_3Doutput, pool1.pool_type);
        //std::cout << "pool1 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool1.CnM2Eigen_3Doutput, conv2.CNM_input);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doConv_CnM(conv2, cnmElements, conv_kernel, channel);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        CnM2Eigen(conv2.CNM_res, conv2.CnM2Eigen_3Doutput);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Normalization(conv2.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv2 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        pool2.CnM2Eigen_3Dinput = conv2.CnM2Eigen_3Doutput; 
        Pooling(&pool2, pool2.CnM2Eigen_3Dinput, pool2.CnM2Eigen_3Doutput, pool2.pool_type);
        //std::cout << "pool2 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(pool2.CnM2Eigen_3Doutput, conv3.CNM_input);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doConv_CnM(conv3, cnmElements, conv_kernel, channel);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        CnM2Eigen(conv3.CNM_res, conv3.CnM2Eigen_3Doutput);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Normalization(conv3.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv3 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv3.CnM2Eigen_3Doutput, conv4.CNM_input);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doConv_CnM(conv4, cnmElements, conv_kernel, channel);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        CnM2Eigen(conv4.CNM_res, conv4.CnM2Eigen_3Doutput);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Normalization(conv4.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv4 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_3D_Matrix(conv4.CnM2Eigen_3Doutput, conv5.CNM_input);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doConv_CnM(conv5, cnmElements, conv_kernel, channel);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        CnM2Eigen(conv5.CNM_res, conv5.CnM2Eigen_3Doutput);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        Normalization(conv5.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
        //std::cout << "conv5 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        pool3.CnM2Eigen_3Dinput = conv5.CnM2Eigen_3Doutput; 
        Pooling(&pool3, pool3.CnM2Eigen_3Dinput, pool3.CnM2Eigen_3Doutput, pool3.pool_type);
        //std::cout << "pool3 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        flatten1.CnM2Eigen_3Dinput = pool3.CnM2Eigen_3Doutput;
        Flatten(flatten1.CnM2Eigen_3Dinput, flatten1.CnM2Eigen_1Doutput);
        //std::cout << "flatten1 " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        initialize_vector(flatten1.CnM2Eigen_1Doutput, dense1.CNM_v1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doFC_CnM(dense1, cnmElements, mvm_kernel, channel);
        //std::cout << "dense 1" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        dense2.CNM_v1 = dense1.CNM_res;
        doFC_CnM(dense2, cnmElements, mvm_kernel, channel);
        //std::cout << "dense 2" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        dense3.CNM_v1 = dense2.CNM_res;
        doFC_CnM(dense3, cnmElements, mvm_kernel, channel);
        //std::cout << "dense 3" << std::endl;
// #ifdef STATS
//         m5_dump_reset_stats(0,0);
// #endif




#endif
    }

    // Finish and clean up.
    m5_dump_stats(0,0);
    m5_exit(0);

    //printVector(dense3.output[T_x-1], 15);

    delete[] conv1.input;
    delete[] pool1.input;
    delete[] conv2.input;
    delete[] pool2.input;
    delete[] conv3.input;
    delete[] conv4.input;
    delete[] conv5.input;
    delete[] pool3.input;
    delete[] flatten1.input;
    delete[] dense1.input;
    delete[] dense2.input;
    delete[] dense3.input;
    delete[] dense3.output;

    return 0;
}
