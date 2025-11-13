/* Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Simple main.cpp tester program for CnM library functions.
 * 
 * Compile with:
 * > g++ -std=c++11 -O3 main.cpp -lpthread -Wall -D(KERNEL) (-DDEBUG -DCHECKER)
 * 
 */

#include "cnm.h"
#include "half.hpp"
#include "pthread.h"
#include <random>

#ifndef CHECKER
#include "gem5/m5ops.h"
#endif

#define NV  4
#define ND  4
#define M   2048
#define N   1000
#define Q   16
#define HI  56
#define WI  56
#define CI  256
#define K   1
#define CO  64
#define PADDING     0
#define STRIDE      1
#define RELU        1
#define NCHANNELS   1
#define SEED    0

// #define DEV_RANGE   4

void fill_vector (std::mt19937& gen, uint64_t* v, uint numVectors, uint vectorDims) {
    std::uniform_real_distribution<float> rng(-10.0, 10.0);
    uint size_64B = div_ceil(numVectors*vectorDims, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != numVectors*vectorDims)
        std::cout << "Warning: numVectors*vectorDims is not aligned with WORDS_PER_64B\n";

    half_float::half* v_half = new half_float::half[round_size];

    // Generate random half precision values
    for (uint i = 0; i < numVectors*vectorDims; i++) {
        v_half[i] = half_float::half(rng(gen));
        // v_half[i] = half_float::half(1.0);  // For testing purposes fill with 1.0
    }
    for (uint i = numVectors*vectorDims; i < round_size; i++) {
        v_half[i] = half_float::half(0.0);
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        v[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            v[i] |= (uint64_t(v_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
        // std::cout << std::showbase << std::hex << v[i] << " ";
    }
}

void fill_vectors (std::mt19937& gen, uint64_t* v1, uint64_t* v2, uint numVectors, uint vectorDims) { 
    std::uniform_real_distribution<float> rng(-10.0, 10.0);
    uint size_64B = div_ceil(numVectors*vectorDims, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != numVectors*vectorDims)
        std::cout << "Warning: numVectors*vectorDims is not aligned with WORDS_PER_64B\n";

    half_float::half* v1_half = new half_float::half[round_size];
    half_float::half* v2_half = new half_float::half[round_size];

    // Generate random half precision values
    for (uint i = 0; i < numVectors*vectorDims; i++) {
        v1_half[i] = rng(gen);
        v2_half[i] = rng(gen);
        // v1_half[i] = half_float::half(1.0); // For testing purposes fill with 1.0
        // v2_half[i] = half_float::half(1.0); // For testing purposes fill with 1.0
    }
    for (uint i = numVectors*vectorDims; i < round_size; i++) {
        v1_half[i] = half_float::half(0.0);
        v2_half[i] = half_float::half(0.0);
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        v1[i] = 0;
        v2[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            v1[i] |= (uint64_t(v1_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
            v2[i] |= (uint64_t(v2_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
        // std::cout << std::showbase << std::hex << v1[i] << " ";
    }
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

void store_vector_float (uint64_t* v, float* v_float, uint numVectors, uint vectorDims) {
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
        v_float[i] = float(v_half[i]);
    }
}

void fill_matrix (std::mt19937& gen, uint64_t* matrix, uint m, uint n) {
    std::uniform_real_distribution<float> rng(-10.0, 10.0);
    uint size_64B = m * div_ceil(n, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    if (round_size != m*n)
        std::cout << "Warning: m*n is not aligned with WORDS_PER_64B\n";

    half_float::half* m_half = new half_float::half[round_size];

    // Generate random half precision values
    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < n; j++) {
            m_half[i*(round_size/m)+j] = rng(gen);
            // m_half[i*(round_size/m)+j] = half_float::half(1.0); // For testing purposes fill with 1.0
            // m_half[i*(round_size/m)+j] = half_float::half(float(i*n)/64+float(j)/64);   // For testing purposes fill with sequential values
        }
        for (uint j = n; j < round_size/m; j++) {
            m_half[i*(round_size/m)+j] = half_float::half(0.0);
        }
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B; i++) {
        matrix[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            matrix[i] |= (uint64_t(m_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

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

void store_matrix_float (uint64_t* matrix, float* matrix_float, uint m, uint n) {
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
            matrix_float[i*n+j] = float(m_half[i*(round_size/m)+j]);
        }
    }
}

void print_input_tensor (uint64_t* tensor, uint height, uint width, uint channels) {
    uint size_64B = channels * div_ceil(height*width, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* tensor_half = new half_float::half[round_size];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((tensor[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            tensor_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    // for (uint i = 0; i < channels; i++) {
    //     std::cout << "Channel " << i << " ------------------\n";
    //     for (uint j = 0; j < height; j++) {
    //         for (uint k = 0; k < width; k++) {
    //             std::cout << tensor_half[i*(round_size/channels) + j*width + k] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }

    for (uint i = 0; i < channels; i++) {
        std::cout << "Channel " << std::dec << i << " ------------------\n";
        for (uint j = 0; j < height; j++) {
            for (uint k = 0; k < width; k++) {
                std::cout << std::hex << tensor_half[i*(round_size/channels) + j*width + k].bin_word() << " ";
            }
            std::cout << std::endl;
        }
    }
}

void print_weights_tensor (uint64_t* tensor, uint numTensors, uint height, uint width, uint channels) {
    uint size_64B = numTensors * div_ceil(channels*height*width, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* tensor_half = new half_float::half[round_size];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((tensor[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            tensor_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    for (uint i = 0; i < numTensors; i++) {
        std::cout << "Tensor " << i << " ------------------\n";
        for (uint j = 0; j < channels; j++) {
            std::cout << "Channel " << j << " ------------------\n";
            for (uint k = 0; k < height; k++) {
                for (uint l = 0; l < width; l++) {
                    // std::cout << tensor_half[i*(round_size/numTensors) + j*(height*width) + k*width + l] << " ";
                    std::cout << tensor_half[i*(round_size/numTensors) + j*(height*width) + k*width + l].bin_word() << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}

// void add_vectors_half (uint64_t* v1, uint64_t* v2, uint64_t* res, uint numVectors, uint vectorDims) {
//     uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
//     uint round_size = size_64B * WORDS_PER_64B;
//     half_float::half* v1_half = new half_float::half[round_size];
//     half_float::half* v2_half = new half_float::half[round_size];
//     half_float::half* res_half = new half_float::half[round_size];

//     for (uint i = 0; i < size_64B; i++) {
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             uint16_t val1 = uint16_t((v1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             uint16_t val2 = uint16_t((v2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             v1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val1);
//             v2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val2);
//             res_half[i*WORDS_PER_64B+j] = v1_half[i*WORDS_PER_64B+j] + v2_half[i*WORDS_PER_64B+j];
//         }
//     }

//     for (uint i = 0; i < size_64B; i++) {
//         res[i] = 0;
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
//         }
//     }
// }

void add_vectors_float (float* v1, float* v2, float* res, uint numVectors, uint vectorDims) {
    for (uint i = 0; i < numVectors*vectorDims; i++) {
        res[i] = v1[i] + v2[i];
    }
}

void dot_product_half (uint64_t* v1, uint64_t* v2, uint64_t* res, uint numVectors, uint vectorDims) {
    uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
    uint round_size = size_64B * WORDS_PER_64B;
    uint size_64B_res = div_ceil(numVectors, WORDS_PER_64B);
    uint round_size_res = size_64B_res * WORDS_PER_64B;
    half_float::half* v1_half = new half_float::half[round_size];
    half_float::half* v2_half = new half_float::half[round_size];
    half_float::half* res_half = new half_float::half[round_size_res];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val1 = uint16_t((v1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            uint16_t val2 = uint16_t((v2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            v1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val1);
            v2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val2);
        }
    }

    for (uint i = 0; i < numVectors; i++) {
        res_half[i] = half_float::half(0.0);
        for (uint j = 0; j < vectorDims; j++) {
            res_half[i] += v1_half[i*vectorDims+j] * v2_half[i*vectorDims+j];
        }
    }

    for (uint i = 0; i < size_64B_res; i++) {
        res[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void partial_dot_product_half (uint64_t* v1, uint64_t* v2, uint64_t* res, uint numVectors, uint vectorDims, uint numPartial) {
    uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
    uint round_size = size_64B * WORDS_PER_64B;
    uint size_64B_res = div_ceil(numVectors, WORDS_PER_64B);
    uint round_size_res = size_64B_res * WORDS_PER_64B;
    half_float::half* v1_half = new half_float::half[round_size];
    half_float::half* v2_half = new half_float::half[round_size];
    half_float::half* res_half = new half_float::half[round_size_res];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val1 = uint16_t((v1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            uint16_t val2 = uint16_t((v2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            v1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val1);
            v2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val2);
        }
    }

    for (uint i = 0; i < numVectors; i++) {
        res_half[i] = half_float::half(0.0);
        for (uint j = 0; j < numPartial; j++) {
            res_half[i] += v1_half[i*vectorDims+j] * v2_half[i*vectorDims+j];
        }
    }

    for (uint i = 0; i < size_64B_res; i++) {
        res[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void dot_product_float (float* v1, float* v2, float* res, uint numVectors, uint vectorDims) {
    for (uint i = 0; i < numVectors; i++) {
        res[i] = 0.0;
        for (uint j = 0; j < vectorDims; j++) {
            res[i] += v1[i*vectorDims+j] * v2[i*vectorDims+j];
        }
    }
}

// void mat_mul (uint64_t* m1, uint64_t* m2, uint64_t* res, uint m, uint n, uint q) {
//     uint size_64B_m1 = m * div_ceil(n, SIMD_WIDTH) * GRF_64B;
//     uint round_size_m1 = size_64B_m1 * WORDS_PER_64B;
//     uint size_64B_m2 = n * div_ceil(q, SIMD_WIDTH) * GRF_64B;
//     uint round_size_m2 = size_64B_m2 * WORDS_PER_64B;
//     uint size_64B_res = m * div_ceil(q, SIMD_WIDTH) * GRF_64B;
//     uint round_size_res = size_64B_res * WORDS_PER_64B;
//     half_float::half* m1_half = new half_float::half[round_size_m1];
//     half_float::half* m2_half = new half_float::half[round_size_m2];
//     half_float::half* res_half = new half_float::half[round_size_res];

//     // Unpack half precision values from 64-bit integers
//     for (uint i = 0; i < size_64B_m1; i++) {
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             uint16_t val = uint16_t((m1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             m1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
//         }
//     }
//     for (uint i = 0; i < size_64B_m2; i++) {
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             uint16_t val = uint16_t((m2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             m2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
//         }
//     }

//     // Compute matrix multiplication
//     for (uint i = 0; i < m; i++) {
//         for (uint j = 0; j < q; j++) {
//             res_half[i*(round_size_res/m)+j] = half_float::half(0.0);
//             for (uint k = 0; k < n; k++) {
//                 res_half[i*(round_size_res/m)+j] += m1_half[i*(round_size_m1/m)+k] * m2_half[k*(round_size_res/m)+j];
//             }
//         }
//     }

//     // Pack half precision values into 64-bit integers
//     for (uint i = 0; i < size_64B_res; i++) {
//         res[i] = 0;
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
//         }
//     }
// }

void mat_mul_float(float* m1, float* m2, float* res, uint m, uint n, uint q) {
    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < q; j++) {
            res[i*q+j] = 0.0;
            for (uint k = 0; k < n; k++) {
                res[i*q+j] += m1[i*n+k] * m2[k*q+j];
            }
        }
    }
}

// void convolution (uint64_t* input, uint64_t* weights, uint64_t* bias, uint64_t* res, uint hi, uint wi, uint ci, uint k, uint co, uint stride, uint padding, bool relu) {
//     uint ho = ((hi + 2*padding - k) / stride) + 1;
//     uint wo = ((wi + 2*padding - k) / stride) + 1;
//     uint size_64B_input = ci*div_ceil(hi*wi, SIMD_WIDTH) * GRF_64B;
//     uint round_size_input = size_64B_input * WORDS_PER_64B;
//     uint size_64B_padded_input = ci*div_ceil((hi+2*padding)*(wi+2*padding), SIMD_WIDTH) * GRF_64B;
//     uint round_size_padded_input = size_64B_padded_input * WORDS_PER_64B;
//     uint size_64B_weights = co*div_ceil(ci*k*k, SIMD_WIDTH) * GRF_64B;
//     uint round_size_weights = size_64B_weights * WORDS_PER_64B;
//     uint size_64B_bias = div_ceil(co, SIMD_WIDTH) * GRF_64B;
//     uint round_size_bias = size_64B_bias * WORDS_PER_64B;
//     uint size_64B_res = co*div_ceil(ho*wo, SIMD_WIDTH) * GRF_64B;
//     uint round_size_res = size_64B_res * WORDS_PER_64B;
//     half_float::half* input_half = new half_float::half[round_size_input];
//     half_float::half* padded_input_half = new half_float::half[round_size_padded_input];
//     half_float::half* weights_half = new half_float::half[round_size_weights];
//     half_float::half* bias_half = new half_float::half[round_size_bias];
//     half_float::half* res_half = new half_float::half[round_size_res];

//     // Unpack half precision values from 64-bit integers
//     for (uint i = 0; i < ci; i++) {
//         for (uint j = 0; j < hi; j++) {
//             for (uint l = 0; l < wi; l++) {
//                 uint16_t val = uint16_t((input[(i*(round_size_input/ci)+j*wi+l)/WORDS_PER_64B] >> (WORD_BITS*((j*wi+l)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1));
//                 input_half[i*hi*wi + j*wi + l] = half_float::half(half_float::detail::binary, val);
//             }
//         }
//     }
//     for (uint i = 0; i < co; i++) {
//         for (uint j = 0; j < ci; j++) {
//             for (uint l = 0; l < k; l++) {
//                 for (uint m = 0; m < k; m++) {
//                     uint16_t val = uint16_t((weights[(i*(round_size_weights/co)+j*k*k+l*k+m)/WORDS_PER_64B] >> (WORD_BITS*((j*k*k+l*k+m)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1));
//                     weights_half[i*k*k*ci + j*k*k + l*k + m] = half_float::half(half_float::detail::binary, val);
//                 }
//             }
//         }
//     }
//     for (uint i = 0; i < size_64B_bias; i++) {
//         for (uint j = 0; j < WORDS_PER_64B; j++) {
//             uint16_t val = uint16_t((bias[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             bias_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
//         }
//     }

//     // Pad input
//     for (uint i = 0; i < round_size_padded_input; i++) {
//         padded_input_half[i] = half_float::half(0.0);
//     }
//     for (uint i = 0; i < ci; i++) {
//         for (uint j = 0; j < hi; j++) {
//             for (uint l = 0; l < wi; l++) {
//                 padded_input_half[i*(hi+2*padding)*(wi+2*padding) + (j+padding)*(wi+2*padding) + l+padding] = input_half[i*hi*wi + j*wi + l];
//             }
//         }
//     }

//     // Compute convolution
//     for (uint i = 0; i < co; i++) {
//         for (uint j = 0; j < ho; j++) {
//             for (uint l = 0; l < wo; l++) {
//                 res_half[i*ho*wo + j*wo + l] = bias_half[i];
//                 for (uint m = 0; m < ci; m++) {
//                     for (uint n = 0; n < k; n++) {
//                         for (uint p = 0; p < k; p++) {
//                             uint inputIdx = m*(hi+2*padding)*(wi+2*padding) + (j*stride+n)*(wi+2*padding) + l*stride+p;
//                             uint weightsIdx = i*k*k*ci + m*k*k + n*k + p;
//                             res_half[i*ho*wo + j*wo + l] += padded_input_half[inputIdx] * weights_half[weightsIdx];       
//                         }
//                     }
//                 }
//                 if (relu) {
//                     res_half[i*ho*wo + j*wo + l] = std::max(res_half[i*ho*wo + j*wo + l], half_float::half(0.0));
//                 }
//             }
//         }
//     }

//     // Pack half precision values into 64-bit integers
//     for (uint i = 0; i < co; i++) {
//         for (uint j = 0; j < div_ceil(wo*ho, WORDS_PER_64B); j++) {
//             res[i*size_64B_res/co + j] = 0;
//             for (uint l = 0; l < WORDS_PER_64B; l++) {
//                 if (j*WORDS_PER_64B + l < wo*ho) {
//                     res[i*size_64B_res/co + j] |= (uint64_t(res_half[i*ho*wo + j*WORDS_PER_64B + l].bin_word()) << (WORD_BITS*l));
//                 } else {    // Add bias to match hardware implementation 
//                     half_float::half val = relu ? std::max(bias_half[i], half_float::half(0.0)) : bias_half[i];
//                     res[i*size_64B_res/co + j] |= (uint64_t(val.bin_word()) << (WORD_BITS*l));
//                 }
//             }
//         }
//         for (uint j = div_ceil(wo*ho, WORDS_PER_64B); j < (size_64B_res/co); j++) { // Add bias to match hardware implementation
//             res[i*size_64B_res/co + j] = 0;
//             for (uint l = 0; l < WORDS_PER_64B; l++) {
//                 half_float::half val = relu ? std::max(bias_half[i], half_float::half(0.0)) : bias_half[i];
//                 res[i*size_64B_res/co + j] |= (uint64_t(val.bin_word()) << (WORD_BITS*l));
//             }
//         }
//     }
// }

void partial_convolution (uint64_t* input, uint64_t* weights, uint64_t* bias, uint64_t* res, uint hi, uint wi, uint ci, uint k, uint co, uint stride, uint padding,
                            uint currentInChannel, uint currentRow, uint currentCol, uint currentOutChannel) {
    uint ho = ((hi + 2*padding - k) / stride) + 1;
    uint wo = ((wi + 2*padding - k) / stride) + 1;
    uint size_64B_input = ci*div_ceil(hi*wi, SIMD_WIDTH) * GRF_64B;
    uint round_size_input = size_64B_input * WORDS_PER_64B;
    uint size_64B_padded_input = ci*div_ceil((hi+2*padding)*(wi+2*padding), SIMD_WIDTH) * GRF_64B;
    uint round_size_padded_input = size_64B_padded_input * WORDS_PER_64B;
    uint size_64B_weights = co*div_ceil(ci*k*k, SIMD_WIDTH) * GRF_64B;
    uint round_size_weights = size_64B_weights * WORDS_PER_64B;
    uint size_64B_bias = div_ceil(co, SIMD_WIDTH) * GRF_64B;
    uint round_size_bias = size_64B_bias * WORDS_PER_64B;
    uint size_64B_res = co*div_ceil(ho*wo, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;
    half_float::half* input_half = new half_float::half[round_size_input];
    half_float::half* padded_input_half = new half_float::half[round_size_padded_input];
    half_float::half* weights_half = new half_float::half[round_size_weights];
    half_float::half* bias_half = new half_float::half[round_size_bias];
    half_float::half* res_half = new half_float::half[round_size_res];

    // Unpack half precision values from 64-bit integers
    for (uint i = 0; i < ci; i++) {
        for (uint j = 0; j < hi; j++) {
            for (uint l = 0; l < wi; l++) {
                uint16_t val = uint16_t((input[(i*(round_size_input/ci)+j*wi+l)/WORDS_PER_64B] >> (WORD_BITS*((j*wi+l)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1));
                input_half[i*hi*wi + j*wi + l] = half_float::half(half_float::detail::binary, val);
            }
        }
    }
    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < ci; j++) {
            for (uint l = 0; l < k; l++) {
                for (uint m = 0; m < k; m++) {
                    uint16_t val = uint16_t((weights[(i*(round_size_weights/co)+j*k*k+l*k+m)/WORDS_PER_64B] >> (WORD_BITS*((j*k*k+l*k+m)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1));
                    weights_half[i*k*k*ci + j*k*k + l*k + m] = half_float::half(half_float::detail::binary, val);
                }
            }
        }
    }
    for (uint i = 0; i < size_64B_bias; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((bias[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            bias_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    // Pad input
    for (uint i = 0; i < round_size_padded_input; i++) {
        padded_input_half[i] = half_float::half(0.0);
    }
    for (uint i = 0; i < ci; i++) {
        for (uint j = 0; j < hi; j++) {
            for (uint l = 0; l < wi; l++) {
                padded_input_half[i*(hi+2*padding)*(wi+2*padding) + (j+padding)*(wi+2*padding) + l+padding] = input_half[i*hi*wi + j*wi + l];
            }
        }
    }

    // Compute convolution until current position
    for (uint i = 0; i < currentOutChannel+1; i++) {
        for (uint j = 0; j < ho; j++) {
            for (uint l = 0; l < wo; l++) {
                res_half[i*ho*wo + j*wo + l] = bias_half[i];
                // Compute first previous complete input channels
                for (uint m = 0; m < currentInChannel; m++) {
                    for (uint n = 0; n < k; n++) {
                        for (uint p = 0; p < k; p++) {
                            uint inputIdx = m*(hi+2*padding)*(wi+2*padding) + (j*stride+n)*(wi+2*padding) + l*stride+p;
                            uint weightsIdx = i*k*k*ci + m*k*k + n*k + p;
                            res_half[i*ho*wo + j*wo + l] += padded_input_half[inputIdx] * weights_half[weightsIdx];
                        }
                    }
                }
                // Compute current input channel until current position, first the rows and then the columns
                for (uint n = 0; n < currentRow; n++) {
                    for (uint p = 0; p < k; p++) {
                        uint inputIdx = currentInChannel*(hi+2*padding)*(wi+2*padding) + (j*stride+n)*(wi+2*padding) + l*stride+p;
                        uint weightsIdx = i*k*k*ci + currentInChannel*k*k + n*k + p;
                        res_half[i*ho*wo + j*wo + l] += padded_input_half[inputIdx] * weights_half[weightsIdx];
                    }
                }
                for (uint p = 0; p <= currentCol; p++) {
                    uint inputIdx = currentInChannel*(hi+2*padding)*(wi+2*padding) + (j*stride+currentRow)*(wi+2*padding) + l*stride+p;
                    uint weightsIdx = i*k*k*ci + currentInChannel*k*k + currentRow*k + p;
                    res_half[i*ho*wo + j*wo + l] += padded_input_half[inputIdx] * weights_half[weightsIdx];
                }
            }
        }
    }

    // Print partial results in binary format
    std::cout << std::endl << "Partial convolution results until ci = " << std::dec << currentInChannel << ", row = " << currentRow << ", col = " << currentCol << std::endl;
    for (uint i = 0; i < co; i++) {
        std::cout << "Output channel " << std::dec << i << ((i < currentOutChannel) ? " completely computed" : ((i == currentOutChannel) ? " partially computed" : "Not computed" )) << " ------------------\n";
        for (uint j = 0; j < ho; j++) {
            for (uint l = 0; l < wo; l++) {
                std::cout << std::showbase << std::hex << res_half[i*ho*wo + j*wo + l].bin_word() << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

void convolution_float (float* input, float* weights, float* bias, float* res, uint hi, uint wi, uint ci, uint k, uint co, uint stride, uint padding, bool relu) {
    uint ho = ((hi + 2*padding - k) / stride) + 1;
    uint wo = ((wi + 2*padding - k) / stride) + 1;

    // Pad input
    float* padded_input = new float[(hi+2*padding)*(wi+2*padding)*ci];
    for (uint i = 0; i < ci; i++) {
        for (uint j = 0; j < hi; j++) {
            for (uint l = 0; l < wi; l++) {
                padded_input[i*(hi+2*padding)*(wi+2*padding) + (j+padding)*(wi+2*padding) + l+padding] = input[i*hi*wi + j*wi + l];
            }
        }
    }

    // Compute convolution
    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < ho; j++) {
            for (uint l = 0; l < wo; l++) {
                res[i*ho*wo + j*wo + l] = bias[i];
                for (uint m = 0; m < k; m++) {
                    for (uint n = 0; n < k; n++) {
                        for (uint p = 0; p < ci; p++) {
                            uint inputIdx = p*(hi+2*padding)*(wi+2*padding) + (j*stride+m)*(wi+2*padding) + l*stride+n;
                            uint weightsIdx = i*k*k*ci + p*k*k + m*k + n;
                            res[i*ho*wo + j*wo + l] += padded_input[inputIdx] * weights[weightsIdx];
                        }
                    }
                }
                if (relu) {
                    res[i*ho*wo + j*wo + l] = std::max(res[i*ho*wo + j*wo + l], 0.0f);
                }
            }
        }
    }
}

// void check_vector_addition_results (uint64_t* res, uint64_t* res_check, uint numVectors, uint vectorDims) {
//     uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
//     uint error = 0;
//     for (uint i = 0; i < size_64B; i++) {
//         if (res[i] != res_check[i]) {
//             std::cout << "Error at position " << std::dec << i << "(" << std::showbase << std::hex << res[i] << " != " << res_check[i] << "):" << std::endl;
//             for (uint j = 0; j < WORDS_PER_64B; j++) {
//                 half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                 half_float::half val2(half_float::detail::binary, uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                 std::cout << val1 << " != " << val2 << std::endl;
//             }
//             error = 1;
//         }
//     }
//     if (!error)
//         std::cout << "Results match!" << std::endl;
// }

void check_dot_product_results (uint64_t* res, uint64_t* res_check, uint numVectors) {
    uint size_64B = div_ceil(numVectors, WORDS_PER_64B);
    uint error = 0;
    for (uint i = 0; i < size_64B; i++) {
        if (res[i] != res_check[i]) {
            std::cout << "Error at position " << std::dec << i << "(" << std::showbase << std::hex << res[i] << " != " << res_check[i] << "):" << std::endl;
            for (uint j = 0; j < WORDS_PER_64B; j++) {
                half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
                half_float::half val2(half_float::detail::binary, uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
                std::cout << val1 << " != " << val2 << std::endl;
            }
            error = 1;
        }
    }
    if (!error)
        std::cout << "Results match!" << std::endl;
}

// void check_mat_mul_results (uint64_t* res, uint64_t* res_check, uint m, uint q) {
//     uint size_64B = m * div_ceil(q, WORDS_PER_64B);
//     uint error = 0;
//     for (uint i = 0; i < size_64B; i++) {
//         if (res[i] != res_check[i]) {
//             std::cout << "Error at position " << std::dec << i << "(" << std::showbase << std::hex << res[i] << " != " << res_check[i] << "):" << std::endl;
//             for (uint j = 0; j < WORDS_PER_64B; j++) {
//                 half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                 half_float::half val2(half_float::detail::binary, uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                 std::cout << val1 << " != " << val2 << std::endl;
//             }
//             error = 1;
//         }
//     }
//     if (!error)
//         std::cout << "Results match!" << std::endl;
// }

// void check_convolution_results (uint64_t* res, uint64_t* res_check, uint ho, uint wo, uint co) {
//     uint size_64B = co * div_ceil(ho*wo, WORDS_PER_64B);
//     uint error = 0;
//     for (uint i = 0; i < size_64B; i++) {
//         if (res[i] != res_check[i]) {
//             // uint rounding_error = true;
//             // for (uint j = 0; j < WORDS_PER_64B; j++) {
//             //     uint16_t val1 = uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             //     uint16_t val2 = uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
//             //     uint16_t lower = (val2 > DEV_RANGE) ? val2 - DEV_RANGE : 0;
//             //     uint16_t upper = (val2 < ((1UL << WORD_BITS)-1) - DEV_RANGE) ? val2 + DEV_RANGE : ((1UL << WORD_BITS)-1);
//             //     if (val1 < lower || val1 > upper) {
//             //         rounding_error = false;
//             //         break;
//             //     }
//             // }
//             // if (!rounding_error) {
//                 std::cout << "Error at position " << std::dec << i << "(" << std::showbase << std::hex << res[i] << " != " << res_check[i];
//                 std::cout << ") at co " << std::dec << i/div_ceil(ho*wo, WORDS_PER_64B) <<", gr " << i%div_ceil(ho*wo, WORDS_PER_64B) << ":" << std::endl;
//                 for (uint j = 0; j < WORDS_PER_64B; j++) {
//                     half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                     half_float::half val2(half_float::detail::binary, uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
//                     std::cout << std::hex << val1 << " != " << val2 << std::endl;
//                 }
//             // }
//             error = 1;
//         }
//     }
//     if (!error)
//         std::cout << "Results match!" << std::endl;
// }

void check_va (uint channel) {
    std::mt19937 gen(SEED);

    std::cout << "Starting vector addition test, V = " << NV << ", n = " << ND << std::endl;

    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B = div_ceil(NV*ND, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;

    uint64_t v1[size_64B];
    uint64_t v2[size_64B];
    uint64_t res[size_64B];
    uint64_t res_check[size_64B];

    float v1_float[round_size];
    float v2_float[round_size];
    float res_float[round_size];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* va_kernel;

    fill_vectors(gen, v1, v2, NV, ND);
    // std::cout << "V1" << std::endl;
    // print_vector(v1, NV, ND);
    // std::cout << "V2" << std::endl;
    // print_vector(v2, NV, ND);


    store_vector_float(v1, v1_float, NV, ND);
    store_vector_float(v2, v2_float, NV, ND);
    // std::cout << "Vector 1 ------------------\n";
    // print_vector(v1, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << v1[i] << std::endl;
    // }
    // std::cout << "Vector 2 ------------------\n";
    // print_vector(v2, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << v2[i] << std::endl;
    // }

    std::cout << "Input vectors generated" << std::endl;

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    add_vectors_half(v1, v2, res_check, NV, ND);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    add_vectors_float(v1_float, v2_float, res_float, NV, ND);
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    std::cout << "Float baseline computed" << std::endl;

    // std::cout << "Result ------------------\n";
    // print_vector(res_check, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << res_check[i] << std::endl;
    // }
    // std::cout << "Result float ------------------\n";
    // for (uint i = 0; i < NV*ND; i++) {
    //     std::cout << res_float[i] << " ";
    // }
    // std::cout << std::endl;

    cnmMemoryMap(cnmElements);

    va_kernel = cnmInitVectorAdditionKernel(cnmElements, channel, NV, ND);
    va_kernel->storeKernel(v1, v2);
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    // cnmComputeKernel(va_kernel);
    va_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va_kernel->loadResults(res);

    check_vector_addition_results(res, res_check, NV, ND);

    // Finished!
    std::cout << "Vector addition test done! V = " << NV << ", n = " << ND << std::endl;

    delete va_kernel;

    std::cout << "Freed kernel\n";

    cnmMemoryUnmap(cnmElements);
}

void check_dp (uint channel) {
    std::mt19937 gen(SEED);

    std::cout << "Starting dot product test, V = " << NV << ", n = " << ND << std::endl;

    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B = div_ceil(NV*ND, SIMD_WIDTH) * GRF_64B;
    uint round_size = size_64B * WORDS_PER_64B;
    uint size_64B_res = div_ceil(NV, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;

    uint64_t v1[size_64B];
    uint64_t v2[size_64B];
    uint64_t res[size_64B_res];
    uint64_t res_check[size_64B_res];

    float v1_float[round_size];
    float v2_float[round_size];
    float res_float[round_size_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* dp_kernel;

    fill_vectors(gen, v1, v2, NV, ND);

    store_vector_float(v1, v1_float, NV, ND);
    store_vector_float(v2, v2_float, NV, ND);
    // std::cout << "Vector 1 ------------------\n";
    // print_vector(v1, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << v1[i] << std::endl;
    // }
    // std::cout << "Vector 2 ------------------\n";
    // print_vector(v2, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << v2[i] << std::endl;
    // }

    std::cout << "Input vectors generated" << std::endl;

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    dot_product_half(v1, v2, res_check, NV, ND);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    dot_product_float(v1_float, v2_float, res_float, NV, ND);
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    std::cout << "Float baseline computed" << std::endl;

    // std::cout << "Partial Result ------------------\n";
    // uint64_t partial_check[size_64B_res];
    // partial_dot_product_half(v1, v2, partial_check, NV, ND, (ND / (((CRF_ENTRIES-4)/4) * 2))* (((CRF_ENTRIES-4)/4) * 2));
    // print_vector(res_check, NV, ND);
    // for (uint i = 0; i < size_64B; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << partial_check[i] << std::endl;
    // }

    // std::cout << "Result ------------------\n";
    // print_vector(res_check, NV, 1);
    // for (uint i = 0; i < size_64B_res; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << res_check[i] << std::endl;
    // }
    // std::cout << "Result float ------------------\n";
    // for (uint i = 0; i < NV; i++) {
    //     std::cout << res_float[i] << " ";
    // }
    // std::cout << std::endl;

    cnmMemoryMap(cnmElements);

    dp_kernel = cnmInitDotProductKernel(cnmElements, channel, NV, ND);
    dp_kernel->storeKernel(v1, v2);
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    // cnmComputeKernel(va_kernel);
    dp_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    dp_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    dp_kernel->loadResults(res);

    check_dot_product_results(res, res_check, NV);

    // Finished!
    std::cout << "Dot product test done! V = " << NV << ", n = " << ND << std::endl;

    delete dp_kernel;

    std::cout << "Freed kernel\n";

    cnmMemoryUnmap(cnmElements);
}

void check_mm (uint channel) {
    std::mt19937 gen(SEED);

    std::cout << "Starting matrix multiplication test, M = " << M << ", N = " << N << ", Q = " << Q << std::endl;

    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B_m1 = M * div_ceil(N, SIMD_WIDTH) * GRF_64B;
    uint round_size_m1 = size_64B_m1 * WORDS_PER_64B;
    uint size_64B_m2 = N * div_ceil(Q, SIMD_WIDTH) * GRF_64B;
    uint round_size_m2 = size_64B_m2 * WORDS_PER_64B;
    uint size_64B_res = M * div_ceil(Q, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;

    uint64_t m1[size_64B_m1];
    uint64_t m2[size_64B_m2];
    uint64_t res[size_64B_res];
    uint64_t res_check[size_64B_res];

    float m1_float[round_size_m1];
    float m2_float[round_size_m2];
    float res_float[round_size_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* mm_kernel;

    fill_matrix(gen, m1, M, N);
    fill_matrix(gen, m2, N, Q);

    store_matrix_float(m1, m1_float, M, N);
    store_matrix_float(m2, m2_float, N, Q);
    // std::cout << "Matrix 1 ------------------\n";
    // print_matrix(m1, M, N);
    // for (uint i = 0; i < M; i++) {
    //     for (uint j = 0; j < size_64B_m1/M; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_m1/M)+j << ": " << std::hex << m1[i*(size_64B_m1/M)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Matrix 2 ------------------\n";
    // print_matrix(m2, N, Q);
    // for (uint i = 0; i < N; i++) {
    //     for (uint j = 0; j < size_64B_m2/N; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_m2/N)+j << ": " << std::hex << m2[i*(size_64B_m2/N)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }

    std::cout << "Input matrices generated" << std::endl;

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    mat_mul(m1, m2, res_check, M, N, Q);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    mat_mul_float(m1_float, m2_float, res_float, M, N, Q);
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    std::cout << "Float baseline computed" << std::endl;

    // std::cout << "Result ------------------\n";
    // print_matrix(res_check, M, Q);
    // for (uint i = 0; i < M; i++) {
    //     for (uint j = 0; j < size_64B_res/M; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_res/M)+j << ": " << std::hex << res_check[i*(size_64B_res/M)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Result float ------------------\n";
    // for (uint i = 0; i < M; i++) {
    //     for (uint j = 0; j < Q; j++) {
    //         std::cout << res_float[i*(round_size_res/M)+j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

    cnmMemoryMap(cnmElements);

    mm_kernel = cnmInitMatrixMultiplicationKernel(cnmElements, channel, M, N, Q);
    mm_kernel->storeKernel(m1, m2);

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    // cnmComputeKernel(mm_kernel);
    mm_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    mm_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    mm_kernel->loadResults(res);

    // std::cout << "CnM output ------------------\n";
    // print_vector(res_check, NV, ND);
    // for (uint i = 0; i < M; i++) {
        // for (uint j = 0; j < size_64B_res/M; j++) {
            // std::cout << std::dec << "pos " << i*(size_64B_res/M)+j << ": " << std::hex << res[i*(size_64B_res/M)+j] << "\t";
        // }
        // std::cout << std::endl;
    // }

    check_mat_mul_results(res, res_check, M, Q);

    // Finished!
    std::cout << "Matrix multiplication test done! M = " << M << ", N = " << N << ", Q = " << Q << std::endl;

    delete mm_kernel;

    std::cout << "Freed kernel\n";

    cnmMemoryUnmap(cnmElements);
}

void check_mvm (uint channel) {
    std::mt19937 gen(SEED);

    std::cout << "Starting matrix vector multiplication test, M = " << M << ", N = " << N << std::endl;

    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B_v1 = div_ceil(M, SIMD_WIDTH) * GRF_64B;
    uint round_size_v1 = size_64B_v1 * WORDS_PER_64B;
    uint size_64B_m2 = M * div_ceil(N, SIMD_WIDTH) * GRF_64B;
    uint round_size_m2 = size_64B_m2 * WORDS_PER_64B;
    uint size_64B_res = div_ceil(N, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;

    uint64_t v1[size_64B_v1];
    uint64_t m2[size_64B_m2];
    uint64_t res[size_64B_res];
    uint64_t res_check[size_64B_res];

    float v1_float[round_size_v1];
    float m2_float[round_size_m2];
    float res_float[round_size_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* mvm_kernel;

    fill_vector(gen, v1, 1, M);
    fill_matrix(gen, m2, M, N);

    store_vector_float(v1, v1_float, 1, M);    
    store_matrix_float(m2, m2_float, M, N);
    // std::cout << "Vector 1 ------------------\n";
    // print_matrix(v1, 1, M);
    // for (uint i = 0; i < size_64B_v1; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << v1[i] << "\t";
    // }
    // std::cout << std::endl;
    // std::cout << "Matrix 2 ------------------\n";
    // print_matrix(m2, M, N);
    // for (uint i = 0; i < M; i++) {
    //     for (uint j = 0; j < size_64B_m2/M; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_m2/M)+j << ": " << std::hex << m2[i*(size_64B_m2/M)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }

    std::cout << "Input matrix and vector generated" << std::endl;

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    mat_mul(v1, m2, res_check, 1, M, N);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    mat_mul_float(v1_float, m2_float, res_float, 1, M, N);
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    std::cout << "Float baseline computed" << std::endl;

    // std::cout << "Result ------------------\n";
    // print_matrix(res_check, 1, N);
    // for (uint i = 0; i < size_64B_res; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << res_check[i] << "\t";
    // }
    // std::cout << std::endl;
    // std::cout << "Result float ------------------\n";
    // for (uint i = 0; i < N; i++) {
    //     std::cout << res_float[i] << " ";
    // }
    // std::cout << std::endl;

    cnmMemoryMap(cnmElements);

    mvm_kernel = cnmInitMatrixVectorMultiplicationKernel(cnmElements, channel, M, N);
    mvm_kernel->storeKernel(v1, m2);

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    // cnmComputeKernel(mm_kernel);
    mvm_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    mvm_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    mvm_kernel->loadResults(res);

    // std::cout << "CnM output ------------------\n";
    // for (uint i = 0; i < size_64B_res; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << res[i] << "\t";
    // }
    // std::cout << std::endl;

    check_mat_mul_results(res, res_check, 1, N);

    // Finished!
    std::cout << "Matrix vector multiplication test done! M = " << M << ", N = " << N << std::endl;

    delete mvm_kernel;

    std::cout << "Freed kernel\n";

    cnmMemoryUnmap(cnmElements);
}

void check_conv (uint channel) {
    std::mt19937 gen(SEED);

    std::cout << "Starting convolution test, HI = " << HI << ", WI = " << WI << ", CI = " << CI << ", K = " << K << ", CO = " << CO << ", STRIDE = " << STRIDE << ", PADDING = " << PADDING << ", RELU = " << RELU << std::endl;
    uint HO = ((HI + 2*PADDING - K) / STRIDE) + 1;
    uint WO = ((WI + 2*PADDING - K) / STRIDE) + 1;

    // Align tensors to the column size, as data is stored and loaded at this chunk size
    uint size_64B_input = CI*div_ceil(HI*WI, SIMD_WIDTH) * GRF_64B;
    uint round_size_input = size_64B_input * WORDS_PER_64B;
    uint size_64B_weights = CO*div_ceil(K*K*CI, SIMD_WIDTH) * GRF_64B;
    uint round_size_weights = size_64B_weights * WORDS_PER_64B;
    uint size_64B_bias = div_ceil(CO, SIMD_WIDTH) * GRF_64B;
    uint round_size_bias = size_64B_bias * WORDS_PER_64B;
    uint size_64B_res = CO*div_ceil(HO*WO, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;
    uint64_t input[size_64B_input];
    uint64_t weights[size_64B_weights];
    uint64_t bias[size_64B_bias];
    uint64_t res[size_64B_res];
    uint64_t res_check[size_64B_res];
    // uint64_t partial_check[size_64B_res];

    float input_float[round_size_input];
    float weights_float[round_size_weights];
    float bias_float[round_size_bias];
    float res_float[round_size_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* conv_kernel;
    fill_matrix(gen, input, CI, HI*WI);
    fill_matrix(gen, weights, CO, K*K*CI);
    fill_vector(gen, bias, 1, CO);

    store_matrix_float(input, input_float, CI, HI*WI);
    store_matrix_float(weights, weights_float, CO, K*K*CI);
    store_vector_float(bias, bias_float, 1, CO);
    // std::cout << "Input tensor ------------------\n";
    // print_input_tensor(input, HI, WI, CI);
    // for (uint i = 0; i < CI; i++) {
    //     for (uint j = 0; j < size_64B_input/CI; j++) {
    //         uint idx = j*WORDS_PER_64B;
    //         std::cout << std::dec << "pos " << idx%WI << "," << idx/HI << "," << i << ": " << std::hex << input[i*(size_64B_input/CI)+j] << std::endl;
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Weights tensor ------------------\n";
    // print_weights_tensor(weights, CO, K, K, CI);
    // for (uint i = 0; i < CO; i++) {
    //     for (uint j = 0; j < size_64B_weights/CO; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_weights/CO)+j << ": " << std::hex << weights[i*(size_64B_weights/CO)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Bias tensor ------------------\n";
    // print_vector(bias, 1, CO);
    // for (uint i = 0; i < size_64B_bias; i++) {
    //     std::cout << std::dec << "pos " << i << ": " << std::hex << bias[i] << std::endl;
    // }

    std::cout << "Input, weight and bias tensors generated" << std::endl;

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    convolution(input, weights, bias, res_check, HI, WI, CI, K, CO, STRIDE, PADDING, RELU);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    convolution_float(input_float, weights_float, bias_float, res_float, HI, WI, CI, K, CO, STRIDE, PADDING, RELU);
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    std::cout << "Float baseline computed" << std::endl;

    // std::cout << "Result ------------------\n";
    // print_input_tensor(res_check, HO, WO, CO);
    // for (uint i = 0; i < CO; i++) {
    //     for (uint j = 0; j < size_64B_res/CO; j++) {
    //         std::cout << std::dec << "pos " << i*(size_64B_res/CO)+j << ": " << std::hex << res_check[i*(size_64B_res/CO)+j] << "\t";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "Result float ------------------\n";
    // for (uint i = 0; i < CO; i++) {
    //     std::cout << "Channel " << i << " ------------------\n";
    //     for (uint j = 0; j < HO; j++) {
    //         for (uint l = 0; l < WO; l++) {
    //             std::cout << res_float[i*HO*WO + j*WO + l] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }

    cnmMemoryMap(cnmElements);

    conv_kernel = cnmInitConvolutionKernel(cnmElements, channel, HI, WI, CI, K, CO, STRIDE, PADDING, RELU);
    conv_kernel->storeKernel(input, weights, bias);

#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    conv_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    conv_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    conv_kernel->loadResults(res);

    check_convolution_results(res, res_check, HO, WO, CO);

    // Finished!
    std::cout << "Convolution test done! HI = " << std::dec << HI << ", WI = " << WI << ", CI = " << CI << ", K = " << K << ", CO = " << CO << ", STRIDE = " << STRIDE << ", PADDING = " << PADDING << ", RELU = " << RELU << std::endl;

    delete conv_kernel;

    std::cout << "Freed kernel\n";

    cnmMemoryUnmap(cnmElements);
}

void check_multich () {
    std::mt19937 gen(SEED);
    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B = div_ceil(NV*ND, SIMD_WIDTH) * GRF_64B;
    uint size_64B_res = div_ceil(NV, SIMD_WIDTH) * GRF_64B;

    uint64_t v1[size_64B], v2[size_64B], v3[size_64B], v4[size_64B], v5[size_64B], v6[size_64B];
    uint64_t res_va1[size_64B], res_va2[size_64B], res_dp[size_64B_res];
    uint64_t res_check_va1[size_64B], res_check_va2[size_64B], res_check_dp[size_64B_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* va1_kernel, *va2_kernel, *dp_kernel;
    cnmMemoryMap(cnmElements);

    fill_vectors(gen, v1, v2, NV, ND);
    fill_vectors(gen, v3, v4, NV, ND);
    fill_vectors(gen, v5, v6, NV, ND);

    std::cout << "Input vectors generated" << std::endl;

    std::cout << "Starting VA1 test" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    add_vectors_half(v1, v2, res_check_va1, NV, ND);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
    va1_kernel = cnmInitVectorAdditionKernel(cnmElements, 0, NV, ND);
    va1_kernel->storeKernel(v1, v2);
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    va1_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va1_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va1_kernel->loadResults(res_va1);
    check_vector_addition_results(res_va1, res_check_va1, NV, ND);
    std::cout << "Vector addition test done! V = " << NV << ", n = " << ND << std::endl;

    std::cout << "Starting DP test" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    dot_product_half(v3, v4, res_check_dp, NV, ND);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
    dp_kernel = cnmInitDotProductKernel(cnmElements, 0, NV, ND);
    dp_kernel->storeKernel(v3, v4);
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    dp_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    dp_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    dp_kernel->loadResults(res_dp);
    check_dot_product_results(res_dp, res_check_dp, NV);
    std::cout << "Dot product test done! V = " << NV << ", n = " << ND << std::endl;

    std::cout << "Starting VA2 test" << std::endl;
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    add_vectors_half(v5, v6, res_check_va2, NV, ND);
#ifndef CHECKER
    m5_dump_stats(0,0);
#endif
    std::cout << "Half baseline computed" << std::endl;
    va2_kernel = cnmInitVectorAdditionKernel(cnmElements, 1, NV, ND);
    va2_kernel->storeKernel(v5, v6);
#ifndef CHECKER
    m5_reset_stats(0,0);
#endif
    va2_kernel->generateSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va2_kernel->executeSequence();
#ifndef CHECKER
    m5_dump_reset_stats(0,0);
#endif
    va2_kernel->loadResults(res_va2);
    check_vector_addition_results(res_va2, res_check_va2, NV, ND);
    std::cout << "Vector addition test done! V = " << NV << ", n = " << ND << std::endl;

    delete va1_kernel;
    delete dp_kernel;
    delete va2_kernel;

    cnmMemoryUnmap(cnmElements);
}

void* exec_sequence_in_thread(void* kernel) {
    Kernel* k = (Kernel*) kernel;
    k->executeSequence();
    return NULL;
}

void check_parallel () {
    std::mt19937 gen(SEED);
    // Align vectors to the column size, as data is stored and loaded at this chunk size
    uint size_64B = div_ceil(NV*ND, SIMD_WIDTH) * GRF_64B;
    uint size_64B_res = div_ceil(NV, SIMD_WIDTH) * GRF_64B;

    uint64_t v1[size_64B], v2[size_64B], v3[size_64B], v4[size_64B], v5[size_64B], v6[size_64B];
    uint64_t res_va1[size_64B], res_va2[size_64B], res_dp[size_64B_res];
    uint64_t res_check_va1[size_64B], res_check_va2[size_64B], res_check_dp[size_64B_res];

    CnmElements* cnmElements = new CnmElements(NCHANNELS);
    Kernel* va1_kernel, *va2_kernel, *dp_kernel;
    cnmMemoryMap(cnmElements);

    fill_vectors(gen, v1, v2, NV, ND);
    fill_vectors(gen, v3, v4, NV, ND);
    fill_vectors(gen, v5, v6, NV, ND);

    std::cout << "Input vectors generated" << std::endl;

    std::cout << "Starting tests" << std::endl;
    add_vectors_half(v1, v2, res_check_va1, NV, ND);
    std::cout << "Half baseline computed" << std::endl;
    va1_kernel = cnmInitVectorAdditionKernel(cnmElements, 0, NV, ND);
    va1_kernel->storeKernel(v1, v2);
    va1_kernel->generateSequence();
    dot_product_half(v3, v4, res_check_dp, NV, ND);
    std::cout << "Half baseline computed" << std::endl;
    dp_kernel = cnmInitDotProductKernel(cnmElements, 0, NV, ND);
    dp_kernel->storeKernel(v3, v4);
    dp_kernel->generateSequence();
    add_vectors_half(v5, v6, res_check_va2, NV, ND);
    std::cout << "Half baseline computed" << std::endl;
    va2_kernel = cnmInitVectorAdditionKernel(cnmElements, 1, NV, ND);
    va2_kernel->storeKernel(v5, v6);
    va2_kernel->generateSequence();
    std::cout << "Sequences generated" << std::endl;

    pthread_t thread_va1, thread_dp, thread_va2;
    pthread_create(&thread_va1, NULL, exec_sequence_in_thread, (void*) va1_kernel);
    pthread_create(&thread_dp, NULL, exec_sequence_in_thread, (void*) dp_kernel);
    pthread_create(&thread_va2, NULL, exec_sequence_in_thread, (void*) va2_kernel);
    pthread_join(thread_va1, NULL);
    pthread_join(thread_dp, NULL);
    pthread_join(thread_va2, NULL);

    va1_kernel->loadResults(res_va1);
    check_vector_addition_results(res_va1, res_check_va1, NV, ND);
    std::cout << "Vector addition test done! V = " << NV << ", n = " << ND << std::endl;
    dp_kernel->loadResults(res_dp);
    check_dot_product_results(res_dp, res_check_dp, NV);
    std::cout << "Dot product test done! V = " << NV << ", n = " << ND << std::endl;
    va2_kernel->loadResults(res_va2);
    check_vector_addition_results(res_va2, res_check_va2, NV, ND);
    std::cout << "Vector addition test done! V = " << NV << ", n = " << ND << std::endl;

    delete va1_kernel;
    delete dp_kernel;
    delete va2_kernel;

    cnmMemoryUnmap(cnmElements);
}

int main (int argc, char * argv[])
{
    std::cout << "Starting CnM check program...\n";

#ifdef VA
    check_va(0);
#endif
#ifdef DP
    check_dp(0);
#endif
#ifdef MM
    check_mm(0);
#endif
#ifdef MVM
    check_mvm(0);
#endif
#ifdef CONV
    check_conv(0);
#endif
#ifdef MULTICH
    check_multich();  
#endif
#ifdef PARALLEL
    check_parallel();
#endif

    return 0;
}