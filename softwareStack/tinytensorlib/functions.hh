/* 
 * Copyright EPFL 2023
 * Joshua Klein
 * 
 * Copyright EPFL 2025
 * Riselda Kodra
 * 
 * This file contains layer utilities, operations, and other functions.
 * - Main operations: convolution, fully-connected
 * - Pooling operations: Max Pool
 * - Normalization operations: LRN
 * - Activation operations: ReLU, Softmax, Sigmoid
 *
 */

#ifndef __FUNCTIONS_HH__
#define __FUNCTIONS_HH__

#include "layer.hh"
#include "utilities.hh"
#ifdef CNM
#include "gem5/m5ops.h"
#endif
/////////////////////////////////////
// Main operation implementations. //
/////////////////////////////////////

// Conv2D implementation.  Extracts patches of input and then performs a MMM
// operation via "contract" or im2col MVMs.
inline void
Convolution2D(conv_layer_args * args, TB_Matrix3D & input,
    TB_Matrix2D & kernels, TB_Matrix3D & output)
{
    // Set up dimensions array for MMM.
    Eigen::array<Eigen::IndexPair<int>, 1> product_dims =
            {Eigen::IndexPair<int>(1, 0)};

    // Extract image patches.
    TB_Matrix2D patches = input.extract_image_patches(args->kernel_h,
        args->kernel_w, args->stride, args->stride, 1, 1, args->padding_type).eval().shuffle(Eigen::array<int, 4>{0, 3, 2, 1}).eval()
        .reshape(Eigen::array<DenseIndex, 2>({args->output_h * args->output_w,
            args->kernel_size}));   //The order of patches dimensions is changed so that the reshaped matrix keeps row-major order

    // Do convolution.
    output = patches.contract(kernels, product_dims)
        .reshape(output.dimensions());

    return;
}

// Fully connected/MVM operation imeplementation.
// TODO: Optimize 3D accesses with vectorization as in previous FullyConnected
// method.
inline void
FullyConnected(fc_layer_args * args, TB_Vector & input,
    TB_Vector & output)
{
    // Set up dimensions array and input for MVM.
    Eigen::array<Eigen::IndexPair<int>, 1> product_dims =
        {Eigen::IndexPair<int>(1, 0)};

    // Do fully digital MVM.
    TB_Matrix2D res = input.reshape(
        Eigen::array<Eigen::Index, 2>{1, args->weights_h});
    output = res.contract(args->weights, product_dims)
        .reshape(output.dimensions());
        
    return;
}

// Pooling operation implementations.
inline void
Pooling(pool_layer_args * args, TB_Matrix3D & input,
    TB_Matrix3D & output, pool_ops_t pool_type)
{
    int i, j, ii, jj, ch, p_s;

    // Dimensions to perform pool on.
    // Dimension 0 is always channel which is not pooled.
    Eigen::array<int, 2> dims({1, 2});

    switch (pool_type) {
        // Defying all logic, this is actually faster than https://github.com/
        // tensorflow/tensorflow/blob/v1.13.1/tensorflow/core/kernels/
        // eigen_pooling.h#L131
        case MAX_POOL_TYPE: {
            for (ch = 0; ch < args->output_c; ch++) {
                for (i = 0, ii = 0; i < args->output_h; i += args->stride,
                    ii++) {
                    for (j = 0, jj = 0; j < args->output_w; j += args->stride,
                        jj++) {
                        TB_Vector tmp = input.slice( // Offsets, Extents
                            Eigen::array<Eigen::Index, 3>{ch, i, j},
                            Eigen::array<Eigen::Index, 3>{1, args->pool_h,
                                args->pool_w}
                        ).maximum(dims);
                        output(ch, ii, jj) = tmp(0);
                    }
                }
            }
            break;
        }
        case AVG_POOL_TYPE: {
            // Note: is susceptible to overflow errors.
            p_s = args->pool_h * args->pool_w;

            for (ch = 0; ch < args->output_c; ch++) {
                for (i = 0, ii = 0; i < args->output_h; i += args->stride,
                    ii++) {
                    for (j = 0, jj = 0; j < args->output_w; j += args->stride,
                        jj++) {
                        TB_Vector tmp = input.slice( // Offsets, Extents
                            Eigen::array<Eigen::Index, 3>{ch, i, j},
                            Eigen::array<Eigen::Index, 3>{1, args->pool_h,
                                args->pool_w}
                        ).sum(dims);
                        output(ch, ii, jj) = tmp(0) / p_s;
                    }
                }
            }
            break;
        }
        default: {break;}
    }

    return;
}

// Flatten operation implementation.
inline void
Flatten(TB_Matrix3D & m, TB_Vector & v)
{
    v = m.reshape(v.dimensions());

    return;
}

// The combination portion of the residual block implementation.
inline void
EndResidual(end_residual_layer_args * args, TB_Matrix3D & input,
    TB_Matrix3D & residual, TB_Matrix3D & output)
{
    output = input + residual;
    return;
}

// DWConv2D brute-force implementation.
inline void
DepthwiseConvolution2D(dwconv_layer_args * args, TB_Matrix3D & input,
    TB_Matrix3D & kernels, TB_Matrix3D & output)
{
    // Lovingly borrowed from https://iq.opengenus.org/depthwise-convolution/.
    // Note: Using extract_image_patches is NOT faster than brute force here.
    for (int out_h = 0; out_h < args->output_h; out_h++) {
        for (int out_w = 0; out_w < args->output_w; out_w++) {
            for (int channel = 0; channel < args->input_c; channel++) {
                int tmp = 0;
                for (int j = 0; j < args->kernel_w; j++) {
                    for (int i = 0; i < args->kernel_h; i++) {
                        tmp += kernels(channel, i, j) * 
                            input(channel, out_h + i, out_w + j);
                    }
                }
#ifndef CNM
                output(channel, out_h, out_w) = tmp;
#else 
                output(channel, out_h, out_w) = static_cast<Eigen::half>(static_cast<float>(tmp));
#endif
            }
        }
    }

    return;
}

// Normalization operation implementations.
inline void
Normalization(TB_Vector & v, norm_ops_t norm_type)
{
    switch(norm_type) {
        case NO_NORM_TYPE: {break;}
        case BATCH_NORM_TYPE: {break;}
        case LRN_NORM_TYPE: {break;}
        default: {break;}
    }

    return;
}

/* 
 * This implements inter-channel LRN in [1], where k = 2, n = 5,
 * Alpha = 1e-5, Beta = 0.75, N = number of kernels.
 *
 * [1] A. Krizhevsky, I. Sutskever, and G. E. Hinton, “ImageNet classification
 * with deep convolutional neural networks,” in NIPS, 2012, pp. 1106–1114.
 */
inline void
Normalization(TB_Matrix3D & m, norm_ops_t norm_type)
{
    int chas = m.dimensions()[0];
    int rows = m.dimensions()[1];
    int cols = m.dimensions()[2];

    switch(norm_type) {
        case NO_NORM_TYPE: {break;}
        case BATCH_NORM_TYPE: {
            
            const int size = chas * rows * cols;
            const float epsilon = 1e-5f;

            TB_Vector v = m.reshape(Eigen::array<DenseIndex, 1>({size}));

            // Compute mean
            float sum = 0.0f;
            for (int i = 0; i < size; ++i) {
                sum += static_cast<float>(v(i));
            }
            float mean = sum / size;

            // Compute variance
            float var_sum = 0.0f;
            for (int i = 0; i < size; ++i) {
                float diff = static_cast<float>(v(i)) - mean;
                var_sum += diff * diff;
            }
            float variance = var_sum / size;

            // Normalize
            for (int i = 0; i < size; ++i) {
                float normalized = (static_cast<float>(v(i)) - mean) / std::sqrt(variance + epsilon);
                v(i) = Eigen::half(normalized); // if using half precision
            }

            m = v.reshape(m.dimensions());
            break;
        }
        case LRN_NORM_TYPE: {
            const int size = chas * rows * cols;
            const double delta = 5.0 / 2.0; // n / 2
            const int upper = chas - 1;

            TB_Vector v = m.reshape(Eigen::array<DenseIndex, 1>({size}));

            for (int i = 0; i < size; i++) {
                double sum = 0;
                int min = (int)fmax(0, i - delta);
                int max = (int)fmin(upper, i + delta);

                // Grab sum.
                for (int j = min; j < max; j++) {
                    sum += static_cast<float>(v(j)) * static_cast<float>(v(i));
                }
                
                float v_i = static_cast<float>(v(i));
                float divisor = pow(2.0f + 1e-5f * sum, 0.75f);
                
                // y[i] = x[i] / (k + alpha * sum)^beta
                //v(i) = v(i) / pow(2 + 1e-5 * sum, 0.75);
                v(i) = Eigen::half(v_i / divisor);
            }

            m = v.reshape(m.dimensions());

            break;
        }
        default: {break;}
    }

    return;
}

// Activation function implementations.
inline void
Activation(TB_Vector & v, act_ops_t act_type)
{
    int size = v.dimensions()[0];

    switch (act_type) {
        case RELU_ACT_TYPE: {
            for (int i = 0; i < size; i++) {
                if (v(i) < 0) {
#ifndef CNM
                    v(i) = 0;
#else
                    v(i) = static_cast<Eigen::half>(static_cast<float>(0));
#endif
                }
            }
            break;
        }
        case SIGMOID_ACT_TYPE: {break;}
        case SOFTMAX_ACT_TYPE: {
            // Not necessarily the most efficient, although correct,
            // implementation.
            // TODO: Make more efficient.
            Tensor<float, 0> expsum = v.cast<float>().exp().sum();
            expsum(0) = 1.0 / expsum(0);
            for (int i = 0; i < size; i++) {
#ifndef CNM
                v(i) = exp(v(i)) * expsum(0);
#else
                v(i) = static_cast<Eigen::half>(static_cast<float>(exp(v(i)) * expsum(0)));
#endif
            }
            break;
        }
        case RELU6_ACT_TYPE: {
            for (int i = 0; i < size; i++) {
                if (v(i) < 0) {
#ifndef CNM
                    v(i) = 0;
#else
                    v(i) = static_cast<Eigen::half>(static_cast<float>(0));
#endif
                } else if (v(i) > 6) {
#ifndef CNM
                    v(i) = 6;
#else
                    v(i) = static_cast<Eigen::half>(static_cast<float>(6));
#endif
                }
            }
            break;   
        }
        default: {break;}
    }

    return;
}

inline void
Activation(TB_Matrix3D & m, act_ops_t act_type)
{
    int chas = m.dimensions()[0];
    int rows = m.dimensions()[1];
    int cols = m.dimensions()[2];

    switch (act_type) {
        case RELU_ACT_TYPE: {
            for (int ch = 0; ch < chas; ch++) {
                for (int i = 0; i < rows; i++) {
                    for (int j = 0; j < cols; j++) {
                        if (m(ch, i, j) < 0) {
#ifndef CNM                            
                            m(ch, i, j) = 0;
#else
                            m(ch, i, j) = static_cast<Eigen::half>(static_cast<float>(0));
#endif                            
                        }
                    }
                }
            }
            break;
        }
        case SIGMOID_ACT_TYPE: {break;}
        case SOFTMAX_ACT_TYPE: {
            // Not necessarily the most efficient, although correct,
            // implementation.
            // TODO: Make more efficient.
            Tensor<float, 0> expsum = m.cast<float>().exp().sum();
            expsum(0) = 1.0 / expsum(0);
            for (int ch = 0; ch < chas; ch++) {
                for (int i = 0; i < rows; i++) {
                    for (int j = 0; j < cols; j++) {
#ifndef CNM                        
                        m(ch, i, j) = exp(m(ch, i, j)) * expsum(0);
#else
                        m(ch, i, j) = static_cast<Eigen::half>(static_cast<float>(exp(m(ch, i, j)) * expsum(0)));
#endif                    
                    }
                }
            }
            break;
        }
        case RELU6_ACT_TYPE: {
            for (int ch = 0; ch < chas; ch++) {
                for (int i = 0; i < rows; i++) {
                    for (int j = 0; j < cols; j++) {
                        if (m(ch, i, j) < 0) {
#ifndef CNM                            
                            m(ch, i, j) = 0;
#else
                            m(ch, i, j) = static_cast<Eigen::half>(static_cast<float>(0));
#endif   
                        } else if (m(ch, i, j) > 6) {
#ifndef CNM                            
                            m(ch, i, j) = 6;
#else
                            m(ch, i, j) = static_cast<Eigen::half>(static_cast<float>(6));
#endif   
                        }
                    }
                }
            }
            break;
        }
        default: {break;}
    }

    return;
}

/////////////////////
// Master methods. //
/////////////////////

inline void
doLayer(conv_layer_args & args, int in_idx=0, int out_idx=0)
{
    Convolution2D(&args, args.input[in_idx], args.weights,
        args.output[out_idx]);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

#ifdef CNM 
inline void
doConv_CnM(conv_layer_args & args, CnmElements* cnmElements, Kernel* kernel, uint channel, int in_idx=0, int out_idx=0)
{ 
// #ifdef STATS
//     //int sys_info = 0;
    // m5_reset_stats(0,0);
// #endif
    kernel = cnmInitConvolutionKernel(cnmElements, channel, args.input_h, args.input_w, args.input_c, args.kernel_h, args.output_c, args.stride, args.padding, args.relu);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->storeKernel(args.CNM_input, args.CNM_weights, args.CNM_bias);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->generateSequence();
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->executeSequence();
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->loadResults(args.CNM_res);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    delete kernel;
    return;
}
#endif
inline void
doLayer(fc_layer_args & args, int in_idx=0, int out_idx=0)
{
    FullyConnected(&args, args.input[in_idx], args.output[out_idx]);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

#ifdef CNM
inline void
doFC_CnM(fc_layer_args & args, CnmElements* cnmElements, Kernel* kernel, uint channel, int in_idx=0, int out_idx=0)
{ 
#ifdef STATS
    //int sys_info = 0;
    // m5_reset_stats(0,0);
#endif
    kernel = cnmInitMatrixVectorMultiplicationKernel(cnmElements, channel, args.weights_h, args.weights_w);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->storeKernel(args.CNM_v1, args.CNM_m2);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->generateSequence();
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->executeSequence();
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    kernel->loadResults(args.CNM_res);
// #ifdef STATS
    // m5_dump_reset_stats(0,0);
// #endif
    delete kernel;
    return;
}
#endif
inline void
doLayer(pool_layer_args & args, int in_idx=0, int out_idx=0)
{
    Pooling(&args, args.input[in_idx], args.output[out_idx], args.pool_type);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

inline void
doLayer(flatten_layer_args & args, int in_idx=0, int out_idx=0)
{
    Flatten(args.input[in_idx], args.output[out_idx]);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

inline void
doLayer(end_residual_layer_args & args, int in_idx=0, int out_idx=0)
{
    EndResidual(&args, args.input[in_idx], args.residual[in_idx],
        args.output[out_idx]);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

#ifdef CNM
inline void
doEndRes_CnM(end_residual_layer_args & args, CnmElements* cnmElements, Kernel* kernel, uint channel, int in_idx=0, int out_idx=0)
{ 
#ifdef STATS
    //int sys_info = 0;
    m5_reset_stats(0,0);
#endif
    kernel = cnmInitVectorAdditionKernel(cnmElements, channel, args.input_c*args.input_h, args.input_w);
// #ifdef STATS
//     m5_dump_reset_stats(0,0);
// #endif
    kernel->storeKernel(args.CNM_v1, args.CNM_v2);
// #ifdef STATS
//     m5_dump_reset_stats(0,0);
// #endif
    kernel->generateSequence();
// #ifdef STATS
//     m5_dump_reset_stats(0,0);
// #endif
    kernel->executeSequence();
// #ifdef STATS
//     m5_dump_reset_stats(0,0);
// #endif
    kernel->loadResults(args.CNM_res);
// #ifdef STATS
//     m5_dump_stats(0,0);
// #endif
    delete kernel;
    return;
}
#endif

inline void
doLayer(dwconv_layer_args & args, int in_idx=0, int out_idx=0)
{
    DepthwiseConvolution2D(&args, args.input[in_idx], args.weights,
        args.output[out_idx]);
    Normalization(args.output[out_idx], args.normalization);
    Activation(args.output[out_idx], args.activation);
    return;
}

#endif // __FUNCTIONS_HH__
