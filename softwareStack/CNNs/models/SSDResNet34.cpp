/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Implementation of a single-core SSD-ResNet34 CNN based [1].
 *
 * [1] https://www.kaggle.com/datasets/pytorch/resnet34
 *
 */

#include <iostream>

#include "tinytensorlib.hh"
#include "model.hh"
#include "gem5/m5ops.h"


void print_matrix (uint64_t* matrix, uint m, uint n) {
    uint size_64B = m * div_ceil(n, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* m_half = new half_float::half[round_size];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((matrix[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            m_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < n; j++) {
            std::cout << m_half[i*(round_size/m)+j] << " ";
        }
        std::cout << std::endl;
    }

    // for (uint i = 0; i < m; i++) {
    //     for (uint j = 0; j < n; j++) {
    //         std::cout << std::hex << m_half[i*(round_size/m)+j].bin_word() << " ";
    //     }
    //     std::cout << std::endl;
    // }
}


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
    conv1.name = "conv1";
    pool1.name = "pool1";
    res2a_branch1.name = "res2a_branch1";
    res2a_branch2a.name = "res2a_branch2a";
    res2a_branch2b.name = "res2a_branch2b";
    res2a_branch2c.name = "res2a_branch2c";
    res2a_end.name = "res2a_end";

    res2b_branch2a.name = "res2b_branch2a";
    res2b_branch2b.name = "res2b_branch2b";
    res2b_branch2c.name = "res2b_branch2c";
    res2b_end.name = "res2b_end";

    res2c_branch2a.name = "res2c_branch2a";
    res2c_branch2b.name = "res2c_branch2b";
    res2c_branch2c.name = "res2c_branch2c";
    res2c_end.name = "res2c_end";

    res3a_branch1.name = "res3a_branch1";
    res3a_branch2a.name = "res3a_branch2a";
    res3a_branch2b.name = "res3a_branch2b";
    res3a_branch2c.name = "res3a_branch2c";
    res3a_end.name = "res3a_end";

    res3b_branch2a.name = "res3b_branch2a";
    res3b_branch2b.name = "res3b_branch2b";
    res3b_branch2c.name = "res3b_branch2c";
    res3b_end.name = "res3b_end";

    res3c_branch2a.name = "res3c_branch2a";
    res3c_branch2b.name = "res3c_branch2b";
    res3c_branch2c.name = "res3c_branch2c";
    res3c_end.name = "res3c_end";

    res3d_branch2a.name = "res3d_branch2a";
    res3d_branch2b.name = "res3d_branch2b";
    res3d_branch2c.name = "res3d_branch2c";
    res3d_end.name = "res3d_end";

    res4a_branch1.name = "res4a_branch1";
    res4a_branch2a.name = "res4a_branch2a";
    res4a_branch2b.name = "res4a_branch2b";
    res4a_branch2c.name = "res4a_branch2c";
    res4a_end.name = "res4a_end";

    res4b_branch2a.name = "res4b_branch2a";
    res4b_branch2b.name = "res4b_branch2b";
    res4b_branch2c.name = "res4b_branch2c";
    res4b_end.name = "res4b_end";

    res4c_branch2a.name = "res4c_branch2a";
    res4c_branch2b.name = "res4c_branch2b";
    res4c_branch2c.name = "res4c_branch2c";
    res4c_end.name = "res4c_end";

    res4d_branch2a.name = "res4d_branch2a";
    res4d_branch2b.name = "res4d_branch2b";
    res4d_branch2c.name = "res4d_branch2c";
    res4d_end.name = "res4d_end";

    res4e_branch2a.name = "res4e_branch2a";
    res4e_branch2b.name = "res4e_branch2b";
    res4e_branch2c.name = "res4e_branch2c";
    res4e_end.name = "res4e_end";

    res4f_branch2a.name = "res4f_branch2a";
    res4f_branch2b.name = "res4f_branch2b";
    res4f_branch2c.name = "res4f_branch2c";
    res4f_end.name = "res4f_end";

    res5a_branch1.name = "res5a_branch1";
    res5a_branch2a.name = "res5a_branch2a";
    res5a_branch2b.name = "res5a_branch2b";
    res5a_branch2c.name = "res5a_branch2c";
    res5a_end.name = "res5a_end";

    res5b_branch2a.name = "res5b_branch2a";
    res5b_branch2b.name = "res5b_branch2b";
    res5b_branch2c.name = "res5b_branch2c";
    res5b_end.name = "res5b_end";

    res5c_branch2a.name = "res5c_branch2a";
    res5c_branch2b.name = "res5c_branch2b";
    res5c_branch2c.name = "res5c_branch2c";
    res5c_end.name = "res5c_end";


    pool2.name = "pool2";
    flatten1.name = "flatten1";
    dense1.name = "dense1";

    // Initialize buffers and other vars.
    //int sys_info = 0;

    generateSingleOutputDataStructure(conv1);
    printLayerInfo(&conv1);

    connectLayers(pool1, conv1);

    connectLayers(res2a_branch1, pool1);
    connectLayers(res2a_branch2a, pool1);
    connectLayers(res2a_branch2b, res2a_branch2a);
    connectLayers(res2a_branch2c, res2a_branch2b);
    connectResidual(res2a_end, res2a_branch1);
    connectLayers(res2a_end, res2a_branch2c);

    connectLayers(res2b_branch2a, res2a_end);
    connectLayers(res2b_branch2b, res2b_branch2a);
    connectLayers(res2b_branch2c, res2b_branch2b);
    connectResidual(res2b_end, res2a_end);
    connectLayers(res2b_end, res2b_branch2c);

    connectLayers(res2c_branch2a, res2b_end);
    connectLayers(res2c_branch2b, res2c_branch2a);
    connectLayers(res2c_branch2c, res2c_branch2b);
    connectResidual(res2c_end, res2b_end);
    connectLayers(res2c_end, res2c_branch2c);

    connectLayers(res3a_branch1, res2c_end);
    connectLayers(res3a_branch2a, res2c_end);
    connectLayers(res3a_branch2b, res3a_branch2a);
    connectLayers(res3a_branch2c, res3a_branch2b);
    connectResidual(res3a_end, res3a_branch1);
    connectLayers(res3a_end, res3a_branch2c);

    connectLayers(res3b_branch2a, res3a_end);
    connectLayers(res3b_branch2b, res3b_branch2a);
    connectLayers(res3b_branch2c, res3b_branch2b);
    connectResidual(res3b_end, res3a_end);
    connectLayers(res3b_end, res3b_branch2c);

    connectLayers(res3c_branch2a, res3b_end);
    connectLayers(res3c_branch2b, res3c_branch2a);
    connectLayers(res3c_branch2c, res3c_branch2b);
    connectResidual(res3c_end, res3b_end);
    connectLayers(res3c_end, res3c_branch2c);

    connectLayers(res3d_branch2a, res3c_end);
    connectLayers(res3d_branch2b, res3d_branch2a);
    connectLayers(res3d_branch2c, res3d_branch2b);
    connectResidual(res3d_end, res3c_end);
    connectLayers(res3d_end, res3d_branch2c);

    connectLayers(res4a_branch1, res3d_end);
    connectLayers(res4a_branch2a, res3d_end);
    connectLayers(res4a_branch2b, res4a_branch2a);
    connectLayers(res4a_branch2c, res4a_branch2b);
    connectResidual(res4a_end, res4a_branch1);
    connectLayers(res4a_end, res4a_branch2c);

    connectLayers(res4b_branch2a, res4a_end);
    connectLayers(res4b_branch2b, res4b_branch2a);
    connectLayers(res4b_branch2c, res4b_branch2b);
    connectResidual(res4b_end, res4a_end);
    connectLayers(res4b_end, res4b_branch2c);

    connectLayers(res4c_branch2a, res4b_end);
    connectLayers(res4c_branch2b, res4c_branch2a);
    connectLayers(res4c_branch2c, res4c_branch2b);
    connectResidual(res4c_end, res4b_end);
    connectLayers(res4c_end, res4c_branch2c);

    connectLayers(res4d_branch2a, res4c_end);
    connectLayers(res4d_branch2b, res4d_branch2a);
    connectLayers(res4d_branch2c, res4d_branch2b);
    connectResidual(res4d_end, res4c_end);
    connectLayers(res4d_end, res4d_branch2c);

    connectLayers(res4e_branch2a, res4d_end);
    connectLayers(res4e_branch2b, res4e_branch2a);
    connectLayers(res4e_branch2c, res4e_branch2b);
    connectResidual(res4e_end, res4d_end);
    connectLayers(res4e_end, res4e_branch2c);

    connectLayers(res4f_branch2a, res4e_end);
    connectLayers(res4f_branch2b, res4f_branch2a);
    connectLayers(res4f_branch2c, res4f_branch2b);
    connectResidual(res4f_end, res4e_end);
    connectLayers(res4f_end, res4f_branch2c);

    connectLayers(res5a_branch1, res4f_end);
    connectLayers(res5a_branch2a, res4f_end);
    connectLayers(res5a_branch2b, res5a_branch2a);
    connectLayers(res5a_branch2c, res5a_branch2b);
    connectResidual(res5a_end, res5a_branch1);
    connectLayers(res5a_end, res5a_branch2c);

    connectLayers(res5b_branch2a, res5a_end);
    connectLayers(res5b_branch2b, res5b_branch2a);
    connectLayers(res5b_branch2c, res5b_branch2b);
    connectResidual(res5b_end, res5a_end);
    connectLayers(res5b_end, res5b_branch2c);

    connectLayers(res5c_branch2a, res5b_end);
    connectLayers(res5c_branch2b, res5c_branch2a);
    connectLayers(res5c_branch2c, res5c_branch2b);
    connectResidual(res5c_end, res5b_end);
    connectLayers(res5c_end, res5c_branch2c);

    connectLayers(pool2, res5c_end); //e korigjova 21/05/2025
    connectLayers(flatten1, pool2);

    dense1.input = flatten1.output;
    printLayerInfo(&dense1);

    // // Do inference.
    //m5_reset_stats(0,0);
    for (int inf = 0; inf < T_x; inf++)
    {  
#ifdef BASELINE 
        //cout << "Inference " << inf << endl;
//#ifdef STATS
        m5_reset_stats(0,0);
//#endif
        doLayer(conv1, inf, 0);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(pool1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(res2a_branch1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res2a_branch2a);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res2a_branch2b);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(res2a_branch2c);
#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(res2a_end);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(res2b_branch2a);
#ifdef STATS
        m5_dump_stats(0,0);
#endif

        doLayer(res2b_branch2b);
        doLayer(res2b_branch2c);
        doLayer(res2b_end);

        doLayer(res2c_branch2a);
        doLayer(res2c_branch2b);
        doLayer(res2c_branch2c);
        doLayer(res2c_end);

#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(res3a_branch1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res3a_branch2a);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res3a_branch2b);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res3a_branch2c);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res3a_end);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(res3b_branch2a);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(res3b_branch2b);
        doLayer(res3b_branch2c);
        doLayer(res3b_end);

        doLayer(res3c_branch2a);
        doLayer(res3c_branch2b);
        doLayer(res3c_branch2c);
        doLayer(res3c_end);

        doLayer(res3d_branch2a);
        doLayer(res3d_branch2b);
        doLayer(res3d_branch2c);
        doLayer(res3d_end);

#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(res4a_branch1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res4a_branch2a);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res4a_branch2b);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res4a_branch2c);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res4a_end);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(res4b_branch2a);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(res4b_branch2b);
        doLayer(res4b_branch2c);
        doLayer(res4b_end);

        doLayer(res4c_branch2a);
        doLayer(res4c_branch2b);
        doLayer(res4c_branch2c);
        doLayer(res4c_end);

        doLayer(res4d_branch2a);
        doLayer(res4d_branch2b);
        doLayer(res4d_branch2c);
        doLayer(res4d_end);

        doLayer(res4e_branch2a);
        doLayer(res4e_branch2b);
        doLayer(res4e_branch2c);
        doLayer(res4e_end);

        doLayer(res4f_branch2a);
        doLayer(res4f_branch2b);
        doLayer(res4f_branch2c);
        doLayer(res4f_end);

#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(res5a_branch1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res5a_branch2a);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res5a_branch2b);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res5a_branch2c);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(res5a_end);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

        doLayer(res5b_branch2a);
#ifdef STATS
        m5_dump_stats(0,0);
#endif
        doLayer(res5b_branch2b);
        doLayer(res5b_branch2c);
        doLayer(res5b_end);

        doLayer(res5c_branch2a);
        doLayer(res5c_branch2b);
        doLayer(res5c_branch2c);
        doLayer(res5c_end);

#ifdef STATS
        m5_reset_stats(0,0);
#endif
        doLayer(pool2);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(flatten1);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
        doLayer(dense1, 0, inf);
//#ifdef STATS
        m5_dump_stats(0,0);
//#endif
#elif defined(CNM)
std::cout << "================================CNM================================" << std::endl;
    m5_reset_stats(0,0);
    cnmMemoryMap(cnmElements);

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
    doConv_CnM(conv1, cnmElements, conv_kernel, channel);
    CnM2Eigen(conv1.CNM_res, conv1.CnM2Eigen_3Doutput);
    Normalization(conv1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    //std::cout << "Conv1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
    
    pool1.CnM2Eigen_3Dinput =  conv1.CnM2Eigen_3Doutput;
    Pooling(&pool1, pool1.CnM2Eigen_3Dinput, pool1.CnM2Eigen_3Doutput, pool1.pool_type);
    //std::cout << "Pool1" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
     
    initialize_3D_Matrix(pool1.CnM2Eigen_3Doutput, res2a_branch1.CNM_input);
    doConv_CnM(res2a_branch1, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2a_branch1.CNM_res, res2a_branch1.CnM2Eigen_3Doutput);
    Normalization(res2a_branch1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
//     std::cout << "res2a_branch1 Norm" << std::endl;
//     printMatrix(res2a_branch1.CnM2Eigen_3Doutput, 0);
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res2a_branch2a.CNM_input = res2a_branch1.CNM_input;
    doConv_CnM(res2a_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2a_branch2a.CNM_res, res2a_branch2a.CnM2Eigen_3Doutput);
    Normalization(res2a_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    //std::cout << "res2a_branch2a Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
    
    initialize_3D_Matrix(res2a_branch2a.CnM2Eigen_3Doutput, res2a_branch2b.CNM_input);
    doConv_CnM(res2a_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2a_branch2b.CNM_res, res2a_branch2b.CnM2Eigen_3Doutput);
    Normalization(res2a_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    //std::cout << "res2a_branch2b Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif
    
    initialize_3D_Matrix(res2a_branch2b.CnM2Eigen_3Doutput, res2a_branch2c.CNM_input);
    doConv_CnM(res2a_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2a_branch2c.CNM_res, res2a_branch2c.CnM2Eigen_3Doutput);
    Normalization(res2a_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
//     std::cout << "res2a_branch2c Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

    initialize_vectors(res2a_branch1.CnM2Eigen_3Doutput, res2a_end.CNM_v2);
    initialize_vectors(res2a_branch2c.CnM2Eigen_3Doutput, res2a_end.CNM_v1);
    doEndRes_CnM(res2a_end, cnmElements, va_kernel, channel);
    std::cout << "res2a_end " << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res2b_branch2a.CNM_input = res2a_end.CNM_res;
    doConv_CnM(res2b_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2b_branch2a.CNM_res, res2b_branch2a.CnM2Eigen_3Doutput);
    Normalization(res2b_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res2b_branch2a Norm" << std::endl;

#ifdef STATS
        m5_dump_stats(0,0);
#endif
    
    initialize_3D_Matrix(res2b_branch2a.CnM2Eigen_3Doutput, res2b_branch2b.CNM_input);
    doConv_CnM(res2b_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2b_branch2b.CNM_res, res2b_branch2b.CnM2Eigen_3Doutput);
    Normalization(res2b_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    //std::cout << "res2b_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res2b_branch2b.CnM2Eigen_3Doutput, res2b_branch2c.CNM_input);
    doConv_CnM(res2b_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2b_branch2c.CNM_res, res2b_branch2c.CnM2Eigen_3Doutput);
    Normalization(res2b_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
     //std::cout << "res2b_branch2c Norm" << std::endl;

    res2b_end.CNM_v2 = res2a_end.CNM_res;   
    initialize_vectors(res2b_branch2c.CnM2Eigen_3Doutput, res2b_end.CNM_v1);
    doEndRes_CnM(res2b_end, cnmElements, va_kernel, channel);
    std::cout << "res2b_end Norm" << std::endl;
    
    res2c_branch2a.CNM_input = res2b_end.CNM_res;
    doConv_CnM(res2c_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2c_branch2a.CNM_res, res2c_branch2a.CnM2Eigen_3Doutput);
    Normalization(res2c_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res2c_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res2c_branch2a.CnM2Eigen_3Doutput, res2c_branch2b.CNM_input);
    doConv_CnM(res2c_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2c_branch2b.CNM_res, res2c_branch2b.CnM2Eigen_3Doutput);
    Normalization(res2c_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res2c_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res2c_branch2b.CnM2Eigen_3Doutput, res2c_branch2c.CNM_input);
    doConv_CnM(res2c_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res2c_branch2c.CNM_res, res2c_branch2c.CnM2Eigen_3Doutput);
    Normalization(res2c_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res2c_branch2c Norm" << std::endl;

    res2c_end.CNM_v2 = res2b_end.CNM_res;
    initialize_vectors(res2c_branch2c.CnM2Eigen_3Doutput, res2c_end.CNM_v1);
    doEndRes_CnM(res2c_end, cnmElements, va_kernel, channel);
    std::cout << "res2c_end Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

    res3a_branch1.CNM_input = res2c_end.CNM_res;
    doConv_CnM(res3a_branch1, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3a_branch1.CNM_res, res3a_branch1.CnM2Eigen_3Doutput);
    Normalization(res3a_branch1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3a_branch1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res3a_branch2a.CNM_input = res2c_end.CNM_res;
    doConv_CnM(res3a_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3a_branch2a.CNM_res, res3a_branch2a.CnM2Eigen_3Doutput);
    Normalization(res3a_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3a_branch2a Norm" << std::endl;

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_3D_Matrix(res3a_branch2a.CnM2Eigen_3Doutput, res3a_branch2b.CNM_input);
    doConv_CnM(res3a_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3a_branch2b.CNM_res, res3a_branch2b.CnM2Eigen_3Doutput);
    Normalization(res3a_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3a_branch2b Norm" << std::endl;

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
    
    initialize_3D_Matrix(res3a_branch2b.CnM2Eigen_3Doutput, res3a_branch2c.CNM_input);
    doConv_CnM(res3a_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3a_branch2c.CNM_res, res3a_branch2c.CnM2Eigen_3Doutput);
    Normalization(res3a_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3a_branch2c Norm" << std::endl;

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_vectors(res3a_branch1.CnM2Eigen_3Doutput, res3a_end.CNM_v2);
    initialize_vectors(res3a_branch2c.CnM2Eigen_3Doutput, res3a_end.CNM_v1);
    doEndRes_CnM(res3a_end, cnmElements, va_kernel, channel);
    std::cout << "res3a_end Norm" << std::endl;

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res3b_branch2a.CNM_input = res3a_end.CNM_res;
    doConv_CnM(res3b_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3b_branch2a.CNM_res, res3b_branch2a.CnM2Eigen_3Doutput);
    Normalization(res3b_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3b_branch2a Norm" << std::endl;

#ifdef STATS
        m5_dump_stats(0,0);
#endif

    initialize_3D_Matrix(res3b_branch2a.CnM2Eigen_3Doutput, res3b_branch2b.CNM_input);
    doConv_CnM(res3b_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3b_branch2b.CNM_res, res3b_branch2b.CnM2Eigen_3Doutput);
    Normalization(res3b_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);

    initialize_3D_Matrix(res3b_branch2b.CnM2Eigen_3Doutput, res3b_branch2c.CNM_input);
    doConv_CnM(res3b_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3b_branch2c.CNM_res, res3b_branch2c.CnM2Eigen_3Doutput);
    Normalization(res3b_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3b_branch2c Norm" << std::endl;

    res3b_end.CNM_v2 = res3a_end.CNM_res;
    initialize_vectors(res3b_branch2c.CnM2Eigen_3Doutput, res3b_end.CNM_v1);
    doEndRes_CnM(res3b_end, cnmElements, va_kernel, channel);
    std::cout << "res3b_end Norm" << std::endl;
    
    res3c_branch2a.CNM_input = res3b_end.CNM_res;
    doConv_CnM(res3c_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3c_branch2a.CNM_res, res3c_branch2a.CnM2Eigen_3Doutput);
    Normalization(res3c_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3c_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res3c_branch2a.CnM2Eigen_3Doutput, res3c_branch2b.CNM_input);
    doConv_CnM(res3c_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3c_branch2b.CNM_res, res3c_branch2b.CnM2Eigen_3Doutput);
    Normalization(res3c_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3c_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res3c_branch2b.CnM2Eigen_3Doutput, res3c_branch2c.CNM_input);
    doConv_CnM(res3c_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3c_branch2c.CNM_res, res3c_branch2c.CnM2Eigen_3Doutput);
    Normalization(res3c_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3c_branch2c Norm" << std::endl;

    res3c_end.CNM_v2 = res3b_end.CNM_res;
    initialize_vectors(res3c_branch2c.CnM2Eigen_3Doutput, res3c_end.CNM_v1);
    doEndRes_CnM(res3c_end, cnmElements, va_kernel, channel);
    std::cout << "res3c_end Norm" << std::endl;
    
    res3d_branch2a.CNM_input = res3c_end.CNM_res;
    doConv_CnM(res3d_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3d_branch2a.CNM_res, res3d_branch2a.CnM2Eigen_3Doutput);
    Normalization(res3d_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3d_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res3d_branch2a.CnM2Eigen_3Doutput, res3d_branch2b.CNM_input);
    doConv_CnM(res3d_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3d_branch2b.CNM_res, res3d_branch2b.CnM2Eigen_3Doutput);
    Normalization(res3d_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3d_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res3d_branch2b.CnM2Eigen_3Doutput, res3d_branch2c.CNM_input);
    doConv_CnM(res3d_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res3d_branch2c.CNM_res, res3d_branch2c.CnM2Eigen_3Doutput);
    Normalization(res3d_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res3d_branch2c Norm" << std::endl;

    res3d_end.CNM_v2 = res3c_end.CNM_res;
    initialize_vectors(res3d_branch2c.CnM2Eigen_3Doutput, res3d_end.CNM_v1);
    doEndRes_CnM(res3d_end, cnmElements, va_kernel, channel);
    std::cout << "res3d_end Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

    res4a_branch1.CNM_input = res3d_end.CNM_res;
    doConv_CnM(res4a_branch1, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4a_branch1.CNM_res, res4a_branch1.CnM2Eigen_3Doutput);
    Normalization(res4a_branch1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4a_branch1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res4a_branch2a.CNM_input = res3d_end.CNM_res;
    doConv_CnM(res4a_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4a_branch2a.CNM_res, res4a_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4a_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4a_branch2a Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_3D_Matrix(res4a_branch2a.CnM2Eigen_3Doutput, res4a_branch2b.CNM_input);
    doConv_CnM(res4a_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4a_branch2b.CNM_res, res4a_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4a_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4a_branch2b Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_3D_Matrix(res4a_branch2b.CnM2Eigen_3Doutput, res4a_branch2c.CNM_input);
    doConv_CnM(res4a_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4a_branch2c.CNM_res, res4a_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4a_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4a_branch2c Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_vectors(res4a_branch1.CnM2Eigen_3Doutput, res4a_end.CNM_v2);
    initialize_vectors(res4a_branch2c.CnM2Eigen_3Doutput, res4a_end.CNM_v1);
    doEndRes_CnM(res4a_end, cnmElements, va_kernel, channel);
    std::cout << "res4a_end" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res4b_branch2a.CNM_input = res4a_end.CNM_res;
    doConv_CnM(res4b_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4b_branch2a.CNM_res, res4b_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4b_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4b_branch2a Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif

    initialize_3D_Matrix(res4b_branch2a.CnM2Eigen_3Doutput, res4b_branch2b.CNM_input);
    doConv_CnM(res4b_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4b_branch2b.CNM_res, res4b_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4b_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4b_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res4b_branch2b.CnM2Eigen_3Doutput, res4b_branch2c.CNM_input);
    doConv_CnM(res4b_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4b_branch2c.CNM_res, res4b_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4b_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4b_branch2c Norm" << std::endl;
    
    res4b_end.CNM_v2 = res4a_end.CNM_res;
    initialize_vectors(res4b_branch2c.CnM2Eigen_3Doutput, res4b_end.CNM_v1);
    doEndRes_CnM(res4b_end, cnmElements, va_kernel, channel);
    std::cout << "res4b_end Norm" << std::endl;
    
    res4c_branch2a.CNM_input = res4b_end.CNM_res;
    doConv_CnM(res4c_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4c_branch2a.CNM_res, res4c_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4c_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4c_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res4c_branch2a.CnM2Eigen_3Doutput, res4c_branch2b.CNM_input);
    doConv_CnM(res4c_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4c_branch2b.CNM_res, res4c_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4c_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4c_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res4c_branch2b.CnM2Eigen_3Doutput, res4c_branch2c.CNM_input);
    doConv_CnM(res4c_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4c_branch2c.CNM_res, res4c_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4c_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4c_branch2c Norm" << std::endl;

    res4c_end.CNM_v2 = res4b_end.CNM_res;
    initialize_vectors(res4c_branch2c.CnM2Eigen_3Doutput, res4c_end.CNM_v1);
    doEndRes_CnM(res4c_end, cnmElements, va_kernel, channel);
    std::cout << "res4c_end Norm" << std::endl;
    
    res4d_branch2a.CNM_input = res4c_end.CNM_res;
    doConv_CnM(res4d_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4d_branch2a.CNM_res, res4d_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4d_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4d_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res4d_branch2a.CnM2Eigen_3Doutput, res4d_branch2b.CNM_input);
    doConv_CnM(res4d_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4d_branch2b.CNM_res, res4d_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4d_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4d_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res4d_branch2b.CnM2Eigen_3Doutput, res4d_branch2c.CNM_input);
    doConv_CnM(res4d_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4d_branch2c.CNM_res, res4d_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4d_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4d_branch2c Norm" << std::endl;

    res4d_end.CNM_v2 = res4c_end.CNM_res;
    initialize_vectors(res4d_branch2c.CnM2Eigen_3Doutput, res4d_end.CNM_v1);
    doEndRes_CnM(res4d_end, cnmElements, va_kernel, channel);
    std::cout << "res4d_end Norm" << std::endl;
    
    res4e_branch2a.CNM_input = res4d_end.CNM_res;
    doConv_CnM(res4e_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4e_branch2a.CNM_res, res4e_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4e_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4e_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res4e_branch2a.CnM2Eigen_3Doutput, res4e_branch2b.CNM_input);
    doConv_CnM(res4e_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4e_branch2b.CNM_res, res4e_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4e_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4e_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res4e_branch2b.CnM2Eigen_3Doutput, res4e_branch2c.CNM_input);
    doConv_CnM(res4e_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4e_branch2c.CNM_res, res4e_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4e_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4e_branch2c Norm" << std::endl;

    res4e_end.CNM_v2 = res4d_end.CNM_res;
    initialize_vectors(res4e_branch2c.CnM2Eigen_3Doutput, res4e_end.CNM_v1);
    doEndRes_CnM(res4e_end, cnmElements, va_kernel, channel);
    std::cout << "res4e_end Norm" << std::endl;
    
    res4f_branch2a.CNM_input = res4e_end.CNM_res;
    doConv_CnM(res4f_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4f_branch2a.CNM_res, res4f_branch2a.CnM2Eigen_3Doutput);
    Normalization(res4f_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4f_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res4f_branch2a.CnM2Eigen_3Doutput, res4f_branch2b.CNM_input);
    doConv_CnM(res4f_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4f_branch2b.CNM_res, res4f_branch2b.CnM2Eigen_3Doutput);
    Normalization(res4f_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4f_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res4f_branch2b.CnM2Eigen_3Doutput, res4f_branch2c.CNM_input);
    doConv_CnM(res4f_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res4f_branch2c.CNM_res, res4f_branch2c.CnM2Eigen_3Doutput);
    Normalization(res4f_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res4f_branch2c Norm" << std::endl;

    res4f_end.CNM_v2 = res4e_end.CNM_res;
    initialize_vectors(res4f_branch2c.CnM2Eigen_3Doutput, res4f_end.CNM_v1);
    doEndRes_CnM(res4f_end, cnmElements, va_kernel, channel);
    std::cout << "res4f_end Norm" << std::endl;
#ifdef STATS
        m5_reset_stats(0,0);
#endif

    res5a_branch1.CNM_input = res4f_end.CNM_res;
    doConv_CnM(res5a_branch1, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5a_branch1.CNM_res, res5a_branch1.CnM2Eigen_3Doutput);
    Normalization(res5a_branch1.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5a_branch1 Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res5a_branch2a.CNM_input = res4f_end.CNM_res;
    doConv_CnM(res5a_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5a_branch2a.CNM_res, res5a_branch2a.CnM2Eigen_3Doutput);
    Normalization(res5a_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5a_branch2a Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_3D_Matrix(res5a_branch2a.CnM2Eigen_3Doutput, res5a_branch2b.CNM_input);
    doConv_CnM(res5a_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5a_branch2b.CNM_res, res5a_branch2b.CnM2Eigen_3Doutput);
    Normalization(res5a_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5a_branch2b Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_3D_Matrix(res5a_branch2b.CnM2Eigen_3Doutput, res5a_branch2c.CNM_input);
    doConv_CnM(res5a_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5a_branch2c.CNM_res, res5a_branch2c.CnM2Eigen_3Doutput);
    Normalization(res5a_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5a_branch2c Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_vectors(res5a_branch1.CnM2Eigen_3Doutput, res5a_end.CNM_v2);
    initialize_vectors(res5a_branch2c.CnM2Eigen_3Doutput, res5a_end.CNM_v1);
    doEndRes_CnM(res5a_end, cnmElements, va_kernel, channel);
    std::cout << "res5a_end Norm" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    res5b_branch2a.CNM_input = res5a_end.CNM_res;
    doConv_CnM(res5b_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5b_branch2a.CNM_res, res5b_branch2a.CnM2Eigen_3Doutput);
    Normalization(res5b_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5b_branch2a Norm" << std::endl;
#ifdef STATS
        m5_dump_stats(0,0);
#endif

    initialize_3D_Matrix(res5b_branch2a.CnM2Eigen_3Doutput, res5b_branch2b.CNM_input);
    doConv_CnM(res5b_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5b_branch2b.CNM_res, res5b_branch2b.CnM2Eigen_3Doutput);
    Normalization(res5b_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5b_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res5b_branch2b.CnM2Eigen_3Doutput, res5b_branch2c.CNM_input);
    doConv_CnM(res5b_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5b_branch2c.CNM_res, res5b_branch2c.CnM2Eigen_3Doutput);
    Normalization(res5b_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5b_branch2c Norm" << std::endl;

    res5b_end.CNM_v2 = res5a_end.CNM_res;
    initialize_vectors(res5b_branch2c.CnM2Eigen_3Doutput, res5b_end.CNM_v1);
    doEndRes_CnM(res5b_end, cnmElements, va_kernel, channel);
    std::cout << "res5b_end Norm" << std::endl;
    
    res5c_branch2a.CNM_input = res5b_end.CNM_res;
    doConv_CnM(res5c_branch2a, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5c_branch2a.CNM_res, res5c_branch2a.CnM2Eigen_3Doutput);
    Normalization(res5c_branch2a.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5c_branch2a Norm" << std::endl;

    initialize_3D_Matrix(res5c_branch2a.CnM2Eigen_3Doutput, res5c_branch2b.CNM_input);
    doConv_CnM(res5c_branch2b, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5c_branch2b.CNM_res, res5c_branch2b.CnM2Eigen_3Doutput);
    Normalization(res5c_branch2b.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);
    std::cout << "res5c_branch2b Norm" << std::endl;

    initialize_3D_Matrix(res5c_branch2b.CnM2Eigen_3Doutput, res5c_branch2c.CNM_input);
    doConv_CnM(res5c_branch2c, cnmElements, conv_kernel, channel);
    CnM2Eigen(res5c_branch2c.CNM_res, res5c_branch2c.CnM2Eigen_3Doutput);
    Normalization(res5c_branch2c.CnM2Eigen_3Doutput, BATCH_NORM_TYPE);

    res5c_end.CNM_v2 = res5b_end.CNM_res;
    initialize_vectors(res5c_branch2c.CnM2Eigen_3Doutput, res5c_end.CNM_v1);
    doEndRes_CnM(res5c_end, cnmElements, va_kernel, channel);
    std::cout << "res5c_end Norm" << std::endl;
       
#ifdef STATS
        m5_reset_stats(0,0);
#endif

    CnM2Eigen(res5c_end.CNM_res, pool2.CnM2Eigen_3Dinput);
    Pooling(&pool2, pool2.CnM2Eigen_3Dinput, pool2.CnM2Eigen_3Doutput, pool2.pool_type);
    std::cout << "Pooling 2" << std::endl;

#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif
    
    flatten1.CnM2Eigen_3Dinput = pool2.CnM2Eigen_3Doutput;
    Flatten(flatten1.CnM2Eigen_3Dinput, flatten1.CnM2Eigen_1Doutput); 
    std::cout << "flatten 1" << std::endl;
#ifdef STATS
        m5_dump_reset_stats(0,0);
#endif

    initialize_vector(flatten1.CnM2Eigen_1Doutput, dense1.CNM_v1);
    doFC_CnM(dense1, cnmElements, mvm_kernel, channel);
    std::cout << "dense 1" << std::endl;
// #ifdef STATS
//         m5_dump_reset_stats(0,0);
// #endif

#endif
    }

    // Finish and clean up.
    m5_dump_stats(0,0);
    m5_exit(0);

    //printVector(dense1.output[T_x-1], 5);
    delete[] conv1.input;
    delete[] pool1.input;

    delete[] res2a_branch2a.input;
    delete[] res2a_branch2b.input;
    delete[] res2a_branch2c.input;
    delete[] res2a_end.residual;
    delete[] res2a_end.input;

    delete[] res2b_branch2a.input;
    delete[] res2b_branch2b.input;
    delete[] res2b_branch2c.input;
    delete[] res2b_end.input;

    delete[] res2c_branch2a.input;
    delete[] res2c_branch2b.input;
    delete[] res2c_branch2c.input;
    delete[] res2c_end.input;

    delete[] res3a_branch2a.input;
    delete[] res3a_branch2b.input;
    delete[] res3a_branch2c.input;
    delete[] res3a_end.residual;
    delete[] res3a_end.input;

    delete[] res3b_branch2a.input;
    delete[] res3b_branch2b.input;
    delete[] res3b_branch2c.input;
    delete[] res3b_end.input;

    delete[] res3c_branch2a.input;
    delete[] res3c_branch2b.input;
    delete[] res3c_branch2c.input;
    delete[] res3c_end.input;

    delete[] res3d_branch2a.input;
    delete[] res3d_branch2b.input;
    delete[] res3d_branch2c.input;
    delete[] res3d_end.input;

    delete[] res4a_branch2a.input;
    delete[] res4a_branch2b.input;
    delete[] res4a_branch2c.input;
    delete[] res4a_end.residual;
    delete[] res4a_end.input;

    delete[] res4b_branch2a.input;
    delete[] res4b_branch2b.input;
    delete[] res4b_branch2c.input;
    delete[] res4b_end.input;

    delete[] res4c_branch2a.input;
    delete[] res4c_branch2b.input;
    delete[] res4c_branch2c.input;
    delete[] res4c_end.input;

    delete[] res4d_branch2a.input;
    delete[] res4d_branch2b.input;
    delete[] res4d_branch2c.input;
    delete[] res4d_end.input;

    delete[] res4e_branch2a.input;
    delete[] res4e_branch2b.input;
    delete[] res4e_branch2c.input;
    delete[] res4e_end.input;

    delete[] res4f_branch2a.input;
    delete[] res4f_branch2b.input;
    delete[] res4f_branch2c.input;
    delete[] res4f_end.input;

    delete[] res5a_branch2a.input;
    delete[] res5a_branch2b.input;
    delete[] res5a_branch2c.input;
    delete[] res5a_end.residual;
    delete[] res5a_end.input;

    delete[] res5b_branch2a.input;
    delete[] res5b_branch2b.input;
    delete[] res5b_branch2c.input;
    delete[] res5b_end.input;

    delete[] res5c_branch2a.input;
    delete[] res5c_branch2b.input;
    delete[] res5c_branch2c.input;
    delete[] res5c_end.input;

    // delete[] pool2.input;
    delete[] flatten1.input;
    delete[] dense1.input;
    delete[] dense1.output;

    return 0;
}
