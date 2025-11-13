/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Implementation of custom single-core LeNet5 CNN based on LeNet5 with MNIST
 * provided by Flavio Ponzina.
 *
 */

#include <iostream>

#include "tinytensorlib.hh"
#include "model.hh"
#include "gem5/m5ops.h"

using namespace std;

#ifdef CNM
void print_vector (uint64_t* v, uint numVectors, uint vectorDims) {
    uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* v_half = new half_float::half[round_size];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((v[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            v_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    for (uint i = 0; i < round_size; i++) {
        // std::cout << v_half[i] << " ";
        std::cout << v_half[i].bin_word() << " ";
    }
    std::cout << std::endl;
}
#endif
int
main(int argc, char * argv[])
{    
#ifdef CNM
    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* conv_kernel;
    Kernel* mvm_kernel;
    uint channel = 0;
    std::cout << MODE_CHANGE_START << std::endl;
    srand(SEED);
#endif

    // Initialize buffers and other vars.
    // int sys_info = 0;

    conv1.output = new TB_Matrix3D[1];
    conv1.output[0] = TB_Matrix3D(
        conv1.output_c, conv1.output_h, conv1.output_w);
    conv1.output[0].setZero();

    pool1.input = conv1.output;
    pool1.output = new TB_Matrix3D[1];
    pool1.output[0] = TB_Matrix3D(
        pool1.output_c, pool1.output_h, pool1.output_w);
    pool1.output[0].setZero();

    conv2.input = pool1.output;
    conv2.output = new TB_Matrix3D[1];
    conv2.output[0] = TB_Matrix3D(
        conv2.output_c, conv2.output_h, conv2.output_w);
    conv2.output[0].setZero();

    pool2.input = conv2.output;
    pool2.output = new TB_Matrix3D[1];
    pool2.output[0] = TB_Matrix3D(
        pool2.output_c, pool2.output_h, pool2.output_w);
    pool2.output[0].setZero();

    conv3.input = pool2.output;
    conv3.output = new TB_Matrix3D[1];
    conv3.output[0] = TB_Matrix3D(
        conv3.output_c, conv3.output_h, conv3.output_w);
    conv3.output[0].setZero();

    flatten1.input = conv3.output;
    flatten1.output = new TB_Vector[1];
    flatten1.output[0] = TB_Vector(flatten1.output_size);
    flatten1.output[0].setZero();

    dense1.input = flatten1.output;
    dense1.output = new TB_Vector[1];
    dense1.output[0] = TB_Vector(dense1.output_size);
    dense1.output[0].setZero();

    dense2.input = dense1.output;

    // Print model information for reference.
    printLayerInfo(&conv1);
    printLayerInfo(&pool1);
    printLayerInfo(&conv2);
    printLayerInfo(&pool2);
    printLayerInfo(&conv3);
    printLayerInfo(&dense1);
    printLayerInfo(&dense2);
    
    // Do inference.
    for (int inf = 0; inf < T_x; inf++)
    {   
        //std::cout << "Inference " << inf << std::endl;
      
        //std::cout << "====================Baseline (Eigen)====================" << std::endl;

//#ifdef STATS
        m5_reset_stats(0,0);
//#endif        
        Convolution2D(&conv1, conv1.input[inf], conv1.weights, conv1.output[0]);
        Normalization(conv1.output[0], LRN_NORM_TYPE);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif        
        Pooling(&pool1, pool1.input[0], pool1.output[0], pool1.pool_type);
        Normalization(pool1.output[0], LRN_NORM_TYPE);
        Activation(pool1.output[0], pool1.activation);

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        Convolution2D(&conv2, conv2.input[0], conv2.weights, conv2.output[0]);
        Normalization(conv2.output[0], LRN_NORM_TYPE);

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        Pooling(&pool2, pool2.input[0], pool2.output[0], pool2.pool_type);
        Normalization(pool2.output[0], LRN_NORM_TYPE);
        Activation(pool2.output[0], pool2.activation);

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        Convolution2D(&conv3, conv3.input[0], conv3.weights, conv3.output[0]);
        Normalization(conv3.output[0], LRN_NORM_TYPE);     
        Activation(conv3.output[0], conv3.activation);

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

        FullyConnected(&dense2, dense2.input[0], dense2.output[inf]);
        Activation(dense2.output[inf], dense2.activation);

//#ifdef STATS
       m5_dump_stats(0,0);
//#endif       
        
#ifdef CNM
        //cout << "====================CnM====================" << endl;

        // ==========================Conv1==========================
//#ifdef STATS
        m5_reset_stats(0,0);
//#endif      
        cnmMemoryMap(cnmElements);       
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel = cnmInitConvolutionKernel(cnmElements, channel, conv1.input_h, conv1.input_w, conv1.input_c, conv1.kernel_h, conv1.output_c, conv1.stride, conv1.padding, conv1.relu); //assumng kernel_h=kernel_w @ToDO later idk if Rafa's code needs to support kernels with unequal dimensions        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->storeKernel(conv1.CNM_input, conv1.CNM_weights, conv1.CNM_bias);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->generateSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->executeSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->loadResults(conv1.CNM_res);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        CnM2Eigen(conv1.CNM_res, conv1.CnM2Eigen_3Doutput);      
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Normalization(conv1.CnM2Eigen_3Doutput, LRN_NORM_TYPE);      
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       


        // ==========================Pool1==========================
        pool1.CnM2Eigen_3Dinput =  conv1.CnM2Eigen_3Doutput;
        // std::cout << "pool1.CnM2Eigen_3Dinput " << std::endl;
        // for(int i = 0; i<pool1.input_c; i++)
        //     printMatrix(pool1.CnM2Eigen_3Dinput, i);           
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Pooling(&pool1, pool1.CnM2Eigen_3Dinput, pool1.CnM2Eigen_3Doutput, pool1.pool_type); 
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Normalization(pool1.CnM2Eigen_3Doutput, LRN_NORM_TYPE);     
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        // ==========================Conv2==========================
        initialize_3D_Matrix(pool1.CnM2Eigen_3Doutput, conv2.CNM_input); 
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel = cnmInitConvolutionKernel(cnmElements, channel, conv2.input_h, conv2.input_w, conv2.input_c, conv2.kernel_h, conv2.output_c, conv2.stride, conv2.padding, conv2.relu);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->storeKernel(conv2.CNM_input, conv2.CNM_weights, conv2.CNM_bias);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->generateSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->executeSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->loadResults(conv2.CNM_res);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        CnM2Eigen(conv2.CNM_res, conv2.CnM2Eigen_3Doutput);      
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Normalization(conv2.CnM2Eigen_3Doutput, LRN_NORM_TYPE);      
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        // ==========================Pool2==========================
        pool2.CnM2Eigen_3Dinput =  conv2.CnM2Eigen_3Doutput;        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif          
        Pooling(&pool2, pool2.CnM2Eigen_3Dinput, pool2.CnM2Eigen_3Doutput, pool2.pool_type); 
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Normalization(pool2.CnM2Eigen_3Doutput, LRN_NORM_TYPE);   
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        // ==========================Conv3==========================
        initialize_3D_Matrix(pool2.CnM2Eigen_3Doutput, conv3.CNM_input); 
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel = cnmInitConvolutionKernel(cnmElements, channel, conv3.input_h, conv3.input_w, conv3.input_c, conv3.kernel_h, conv3.output_c, conv3.stride, conv3.padding, conv3.relu);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->storeKernel(conv3.CNM_input, conv3.CNM_weights, conv3.CNM_bias);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->generateSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->executeSequence();        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        conv_kernel->loadResults(conv3.CNM_res);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif          
        CnM2Eigen(conv3.CNM_res, conv3.CnM2Eigen_3Doutput);      
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        Normalization(conv3.CnM2Eigen_3Doutput, LRN_NORM_TYPE);   
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif  

        // ==========================Flatten1==========================
        flatten1.CnM2Eigen_3Dinput = conv3.CnM2Eigen_3Doutput;           
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif  
        Flatten(flatten1.CnM2Eigen_3Dinput, flatten1.CnM2Eigen_1Doutput);  
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif  

        // ==========================Dense1==========================
        initialize_vector(flatten1.CnM2Eigen_1Doutput, dense1.CNM_v1);        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel = cnmInitMatrixVectorMultiplicationKernel(cnmElements, channel, dense1.weights_h, dense1.weights_w);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->storeKernel(dense1.CNM_v1, dense1.CNM_m2);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->generateSequence();
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->executeSequence();
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->loadResults(dense1.CNM_res);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       

        // ==========================Dense2==========================
        dense2.CNM_v1 = dense1.CNM_res;        
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel = cnmInitMatrixVectorMultiplicationKernel(cnmElements, channel, dense2.weights_h, dense2.weights_w);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->storeKernel(dense2.CNM_v1, dense2.CNM_m2);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->generateSequence();
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->executeSequence();
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif       
        mvm_kernel->loadResults(dense2.CNM_res);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif                 

#endif        
    }
    
    // Finish and clean up.
    m5_dump_stats(0,0);
    m5_exit(0);

    delete[] conv1.input;
    delete[] pool1.input;
    delete[] conv2.input;
    delete[] pool2.input;
    delete[] conv3.input;
    delete[] flatten1.input;
    delete[] dense1.input;
    delete[] dense2.input;
    delete[] dense2.output;

    return 0;
}
