/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Defines, includes, and functions to configure and employ the Compute-near-Memory capabilities.
 *
 */

#ifndef CNM_UTILS_H
#define CNM_UTILS_H

#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>

// Includes to select and comply with PU and DRAM configuration
#include "defs.h"
#include "opcodes.h"
#include "cnm_intrinsics.h"
#include "half.hpp"

// Defines to describe the CnM structure
#define GEM5_OFFSET     0x400000000
#define EXEC_INST_MEM   GEM5_OFFSET
#define RF_OFFSET       (1UL << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS - 1))
#define RF_START        (GEM5_OFFSET + RF_OFFSET)
#define RF_INST_MEM     RF_START
#define LENGTH_MEM      (RF_INST_MEM - EXEC_INST_MEM)

// Addresses to change between memory and PIM modes for the different channels
#define MODE_CHANGE_START   (RF_OFFSET - 1 - ((1UL << (GLOBAL_OFFSET + CHANNEL_BITS)) - 1))
#define MODE_CHANGE_END     (MODE_CHANGE_START + (((1UL << (CHANNEL_BITS)) - 1) << GLOBAL_OFFSET))

#define MASK_BANK_LSB   (1UL << GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS)                              // Keeps least significant bank bit
#define MASK_BABG_BITS  (~(((1UL << (BG_BITS + BANK_BITS)) - 1) << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS)))  // Sets to 0 bank and bank-group bits
#define NUM_COL         (1 << (COL_BITS))
#define NUM_ROW         (1 << (ROW_BITS))
#define NUM_BANK        (1 << (BANK_BITS))
#define NUM_BG          (1 << (BG_BITS))
#define SHIFT_CH        GLOBAL_OFFSET  // Shifts to the channel bits
#define SHIFT_COL       (GLOBAL_OFFSET + CHANNEL_BITS)  // Shifts to the column bits
#define SHIFT_RANK      (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS)  // Shifts to the rank bits
#define SHIFT_BG        (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS)   // Shifts to the bg bits
#define SHIFT_BANK      (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS) // Shifts to the bank bits
#define SHIFT_ROW       (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS) // Shifts to the row bits

#define CRF_OFFSET      (RF_CRF << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))
#define SRFM_OFFSET     (RF_SRF_M << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))
#define SRFA_OFFSET     (RF_SRF_A << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))
#define GRFA_OFFSET     (RF_GRF_A << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))
#define GRFB_OFFSET     (RF_GRF_B << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))
#define GRFB_OFFEND     ((RF_GRF_B+1UL) << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS))

enum class Level : int {Channel, Rank, BankGroup, Bank, Row, Column, MAX};
std::string level_str[int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};
uint addr_bits[int(Level::MAX)] = {CHANNEL_BITS, RANK_BITS, BG_BITS, BANK_BITS, ROW_BITS, COL_BITS};
Level map[int(Level::MAX)] = {Level::Channel, Level::Column, Level::Rank,
                        Level::BankGroup, Level::Bank, Level::Row};

#define GRF_64B         (GRF_WIDTH/64)  // Number of 64B words in a GRF entry
#define WORDS_PER_64B   (64/WORD_BITS)  // Number of words in a 64B word

// Function to memory map a section of the physical memory
// Parameters:
// - length: length of the memory region to map
// - offset: physical address where the memory region should start
uint64_t* memoryMap(uint64_t length, uint64_t offset) {
    int fd;
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        printf("ERR: cannot open /dev/mem\n");
        return (uint64_t *)-1;
    }

    uint64_t* memPtr = (uint64_t*) mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

    if (memPtr == MAP_FAILED) {
        printf("error is %d\n", errno);
        perror("Cannot MAP\n");
        return NULL;
    }

    return memPtr;
}

// Function to unmap a section of the physical memory
void memoryUnmap(uint64_t* addr, size_t length) {
    if (munmap(addr, length)) {
        printf("ERR\n");
    }
}

inline void switchCnmMode (uint channel, uint64_t* data, uint64_t* rfAddr) {
    uint64_t* addr = rfAddr + (MODE_CHANGE_START + (channel << GLOBAL_OFFSET))/8;
    ldrData(addr, data);
#ifdef DEBUG
    std::cout << "Switched between memory modes for channel " << channel << " at address " << addr << std::endl;
#endif
}

// Function to generate the address of a memory location in the CNM DRAM memory mapped region
inline uint64_t* cnmExecAddress(uint chIdx, uint raIdx, uint bgIdx, uint baIdx, uint roIdx, uint coIdx, uint64_t* offsetAddr) {
    return offsetAddr + (
            (chIdx << SHIFT_CH) + 
            (raIdx << SHIFT_RANK) + 
            (bgIdx << SHIFT_BG) + 
            (baIdx << SHIFT_BANK) + 
            (roIdx << SHIFT_ROW) + 
            (coIdx << SHIFT_COL)
        ) / sizeof(uint64_t);
}

#ifdef DEBUG
// Function to generate the address of a memory location in the CNM DRAM memory mapped region, taking a std::vector as input
uint64_t* cnmExecAddress(std::vector<uint> addrVec, uint64_t* offsetAddr) {
    uint64_t address = 0;
    uint64_t offset = GLOBAL_OFFSET;

    for (int i = 0; i < int(Level::MAX); i++) {
        if (addr_bits[int(map[i])]) {
            if (addrVec[int(map[i])] >= (1U << addr_bits[int(map[i])])) {
                std::cout << "Error, " << level_str[int(map[i])] << " index is too large: " << addrVec[int(map[i])] << std::endl;
                return NULL;
            }
            if (map[i] == Level::Row && addrVec[int(map[i])] >= (1U << (addr_bits[int(map[i])]-1))) {
                std::cout << "Error, row MSB is set, which will interfere with PIM computing" << std::endl;
                return NULL;
            }
            address |= (addrVec[int(map[i])] << offset);
            offset += addr_bits[int(map[i])];
        }
    }
    return offsetAddr + address/sizeof(uint64_t);
}
#endif

uint64_t offsetBgBankRowCol(uint bgIdx, uint bankIdx, uint rowIdx, uint colIdx) {
    return ((bgIdx << SHIFT_BG) + (bankIdx << SHIFT_BANK) + (rowIdx << SHIFT_ROW) + (colIdx << SHIFT_COL))/8;
}

// Function to advance address indeces in columns
void nextCol(uint* rowIdx, uint* colIdx) {
    if (++(*colIdx) == NUM_COL) {
        *colIdx = 0;
        (*rowIdx)++;
    }
}

void nextColChangeParity (uint* rowIdx, uint* colIdx, uint* parity) {
    if (*parity % 2) {
        nextCol(rowIdx, colIdx);
    }
    *parity = (*parity + 1) % 2;
}

// Function to reduce address indeces in columns
void reduceColIdx(uint* rowIdx, uint* colIdx, uint jump) {
    if (*colIdx >= jump) {
        *colIdx -= jump;
    } else {
        uint absCol = *rowIdx * NUM_COL + *colIdx;
        absCol -= jump;
        *rowIdx = absCol / NUM_COL;
        *colIdx = absCol % NUM_COL;
    }
}


// Function to reduce address indeces in columns and change parity, aligning to the first bank and bank group
void changeParityJumpBackCol(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx, uint* parity, uint jump) {
    if (*bgIdx != 0 || *bankIdx != 0) {
        nextCol (rowIdx, colIdx);
    }
    *bgIdx = 0;
    *bankIdx = 0;
    if (*parity % 2 == 0) {
        reduceColIdx(rowIdx, colIdx, jump);
    }
    *parity = (*parity + 1) % 2;
}

// Function to advance address indeces in column chunks, increasing in order bg, bank, column and row
void nextColChunkBgBa(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx) {
    // The sequence of bankIdx should be 0, 2, ... NUM_BANK-2, 1, 3, ... NUM_BANK-1
    if (++(*bgIdx) == NUM_BG) {
        *bgIdx = 0;
        *bankIdx += 2;
        if (*bankIdx >= NUM_BANK) {
            if (*bankIdx % 2) { // Run through all banks, advance to next column
                *bankIdx = 0;
                if (++(*colIdx) == NUM_COL) {
                    *colIdx = 0;
                    (*rowIdx)++;
                }
            } else {    // Run through even banks, advance to odd banks
                *bankIdx = 1;
            }
        }
    }
}

// Function to advance address indeces in column chunks, increasing in order bank, bg, column and row
void nextColChunkBaBg(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx) {
    if (++(*bankIdx) == NUM_BANK) {
        *bankIdx = 0;
        if (++(*bgIdx) == NUM_BG) {
            *bgIdx = 0;
            if (++(*colIdx) == NUM_COL) {
                *colIdx = 0;
                (*rowIdx)++;
            }
        }
    }
}

// Function to advance address indeces in column chunks, increasing in order bg, 2 bank, column and row
void nextColChunkBgBaKeepBankParity(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx) {
    // The sequence of bankIdx should be 0, 2, ... NUM_BANK-2 or 1, 3, ... NUM_BANK-1
    if (++(*bgIdx) == NUM_BG) {
        *bgIdx = 0;
        *bankIdx += 2;
        if (*bankIdx >= NUM_BANK) {
            *bankIdx = *bankIdx % 2;
            if (++(*colIdx) == NUM_COL) {
                *colIdx = 0;
                (*rowIdx)++;
            }
        }
    }
}

// Function to advance address indeces in column chunks, increasing in order bg, bank, and reducing column and row unless all banks are accessed
void nextBgBankJumpBackCol(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx, uint jump) {
    // The sequence of bankIdx should be 0, 2, ... NUM_BANK-2, 1, 3, ... NUM_BANK-1
    if (++(*bgIdx) == NUM_BG) {
        *bgIdx = 0;
        *bankIdx += 2;
        if (*bankIdx >= NUM_BANK) {
            if (*bankIdx % 2) {
                *bankIdx = 0;
                return; // Already ran through all banks, continue in this column
            } else {    // Run through even banks, advance to odd banks
                *bankIdx = 1;
            }
        }
    }
    reduceColIdx(rowIdx, colIdx, jump);
}

// Function to advance address indeces in column chunks, increasing in order bg, bank, and reducing column and row unless all banks are accessed
void nextBgBankJumpBackColKeepParity(uint* bgIdx, uint* bankIdx, uint* rowIdx, uint* colIdx, uint bankParity, uint jump) {
    if (++(*bgIdx) == NUM_BG) {
        *bgIdx = 0;
        *bankIdx += 2;
        if (*bankIdx >= NUM_BANK) {
            *bankIdx = *bankIdx % 2;
            if (bankParity) {   // Number of dimensions is odd, advance to next column
                if (++(*colIdx) == NUM_COL) {
                    *colIdx = 0;
                    (*rowIdx)++;
                }
            }
            return;
        }
    }
    reduceColIdx(rowIdx, colIdx, jump);
}

uint64_t offsetBankRowCol(uint bankIdx, uint rowIdx, uint colIdx) {
    return ((bankIdx << SHIFT_BANK) + (rowIdx << SHIFT_ROW) + (colIdx << SHIFT_COL))/8;
}

// Function to advance column and row indeces when accessing all banks in parallel
void nextColAllBanks(uint* rowIdx, uint* colIdx) {
    if (++(*colIdx) == NUM_COL) {
        *colIdx = 0;
        (*rowIdx)++;
    }
}

// Function to jump a number of columns and rows when accessing all banks in parallel
void jumpColAllBanks(uint* rowIdx, uint* colIdx, uint jump) {
    *colIdx += jump;
    if (*colIdx >= NUM_COL) {
        *rowIdx += *colIdx / NUM_COL;
        *colIdx = *colIdx % NUM_COL;
    }
}

inline uint div_ceil(uint dividend, uint divisor) {
    return (dividend + divisor - 1) / divisor;
}

void convolution (uint64_t* input, uint64_t* weights, uint64_t* bias, uint64_t* res, uint hi, uint wi, uint ci, uint k, uint co, uint stride, uint padding, bool relu) {
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

    // Compute convolution
    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < ho; j++) {
            for (uint l = 0; l < wo; l++) {
                res_half[i*ho*wo + j*wo + l] = bias_half[i];
                for (uint m = 0; m < ci; m++) {
                    for (uint n = 0; n < k; n++) {
                        for (uint p = 0; p < k; p++) {
                            uint inputIdx = m*(hi+2*padding)*(wi+2*padding) + (j*stride+n)*(wi+2*padding) + l*stride+p;
                            uint weightsIdx = i*k*k*ci + m*k*k + n*k + p;
                            res_half[i*ho*wo + j*wo + l] += padded_input_half[inputIdx] * weights_half[weightsIdx];       
                        }
                    }
                }
                if (relu) {
                    res_half[i*ho*wo + j*wo + l] = std::max(res_half[i*ho*wo + j*wo + l], half_float::half(0.0));
                }
            }
        }
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < div_ceil(wo*ho, WORDS_PER_64B); j++) {
            res[i*size_64B_res/co + j] = 0;
            for (uint l = 0; l < WORDS_PER_64B; l++) {
                if (j*WORDS_PER_64B + l < wo*ho) {
                    res[i*size_64B_res/co + j] |= (uint64_t(res_half[i*ho*wo + j*WORDS_PER_64B + l].bin_word()) << (WORD_BITS*l));
                } else {    // Add bias to match hardware implementation 
                    half_float::half val = relu ? std::max(bias_half[i], half_float::half(0.0)) : bias_half[i];
                    res[i*size_64B_res/co + j] |= (uint64_t(val.bin_word()) << (WORD_BITS*l));
                }
            }
        }
        for (uint j = div_ceil(wo*ho, WORDS_PER_64B); j < (size_64B_res/co); j++) { // Add bias to match hardware implementation
            res[i*size_64B_res/co + j] = 0;
            for (uint l = 0; l < WORDS_PER_64B; l++) {
                half_float::half val = relu ? std::max(bias_half[i], half_float::half(0.0)) : bias_half[i];
                res[i*size_64B_res/co + j] |= (uint64_t(val.bin_word()) << (WORD_BITS*l));
            }
        }
    }
}

void check_convolution_results (uint64_t* res, uint64_t* res_check, uint ho, uint wo, uint co) {
    uint size_64B = co * div_ceil(ho*wo, WORDS_PER_64B);
    uint error = 0;
    for (uint i = 0; i < size_64B; i++) {
        if (res[i] != res_check[i]) {
            // uint rounding_error = true;
            // for (uint j = 0; j < WORDS_PER_64B; j++) {
            //     uint16_t val1 = uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            //     uint16_t val2 = uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            //     uint16_t lower = (val2 > DEV_RANGE) ? val2 - DEV_RANGE : 0;
            //     uint16_t upper = (val2 < ((1UL << WORD_BITS)-1) - DEV_RANGE) ? val2 + DEV_RANGE : ((1UL << WORD_BITS)-1);
            //     if (val1 < lower || val1 > upper) {
            //         rounding_error = false;
            //         break;
            //     }
            // }
            // if (!rounding_error) {
                std::cout << "Error at position " << std::dec << i << "(" << std::showbase << std::hex << res[i] << " != " << res_check[i];
                std::cout << ") at co " << std::dec << i/div_ceil(ho*wo, WORDS_PER_64B) <<", gr " << i%div_ceil(ho*wo, WORDS_PER_64B) << ":" << std::endl;
                for (uint j = 0; j < WORDS_PER_64B; j++) {
                    half_float::half val1(half_float::detail::binary, uint16_t((res[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
                    half_float::half val2(half_float::detail::binary, uint16_t((res_check[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1)));
                    std::cout << std::hex << val1 << " != " << val2 << std::endl;
                }
            // }
            error = 1;
        }
    }
    if (!error)
        std::cout << "Results match!" << std::endl;
}

void mat_mul (uint64_t* m1, uint64_t* m2, uint64_t* res, uint m, uint n, uint q) {
    uint size_64B_m1 = m * div_ceil(n, SIMD_WIDTH) * GRF_64B;
    uint round_size_m1 = size_64B_m1 * WORDS_PER_64B;
    uint size_64B_m2 = n * div_ceil(q, SIMD_WIDTH) * GRF_64B;
    uint round_size_m2 = size_64B_m2 * WORDS_PER_64B;
    uint size_64B_res = m * div_ceil(q, SIMD_WIDTH) * GRF_64B;
    uint round_size_res = size_64B_res * WORDS_PER_64B;
    half_float::half* m1_half = new half_float::half[round_size_m1];
    half_float::half* m2_half = new half_float::half[round_size_m2];
    half_float::half* res_half = new half_float::half[round_size_res];

    // Unpack half precision values from 64-bit integers
    for (uint i = 0; i < size_64B_m1; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((m1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            m1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }
    for (uint i = 0; i < size_64B_m2; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val = uint16_t((m2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            m2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val);
        }
    }

    // Compute matrix multiplication
    for (uint i = 0; i < m; i++) {
        for (uint j = 0; j < q; j++) {
            res_half[i*(round_size_res/m)+j] = half_float::half(0.0);
            for (uint k = 0; k < n; k++) {
                res_half[i*(round_size_res/m)+j] += m1_half[i*(round_size_m1/m)+k] * m2_half[k*(round_size_res/m)+j];
            }
        }
    }

    // Pack half precision values into 64-bit integers
    for (uint i = 0; i < size_64B_res; i++) {
        res[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void check_mat_mul_results (uint64_t* res, uint64_t* res_check, uint m, uint q) {
    uint size_64B = m * div_ceil(q, WORDS_PER_64B);
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

void add_vectors_half (uint64_t* v1, uint64_t* v2, uint64_t* res, uint numVectors, uint vectorDims) {
    uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
    uint round_size = size_64B * WORDS_PER_64B;
    half_float::half* v1_half = new half_float::half[round_size];
    half_float::half* v2_half = new half_float::half[round_size];
    half_float::half* res_half = new half_float::half[round_size];

    for (uint i = 0; i < size_64B; i++) {
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            uint16_t val1 = uint16_t((v1[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            uint16_t val2 = uint16_t((v2[i] >> (WORD_BITS*j)) & ((1UL << WORD_BITS) - 1));
            v1_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val1);
            v2_half[i*WORDS_PER_64B+j] = half_float::half(half_float::detail::binary, val2);
            res_half[i*WORDS_PER_64B+j] = v1_half[i*WORDS_PER_64B+j] + v2_half[i*WORDS_PER_64B+j];
        }
    }

    for (uint i = 0; i < size_64B; i++) {
        res[i] = 0;
        for (uint j = 0; j < WORDS_PER_64B; j++) {
            res[i] |= (uint64_t(res_half[i*WORDS_PER_64B+j].bin_word()) << (WORD_BITS*j));
        }
    }
}

void check_vector_addition_results (uint64_t* res, uint64_t* res_check, uint numVectors, uint vectorDims) {
    uint size_64B = div_ceil(numVectors*vectorDims, WORDS_PER_64B);
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

#endif  // CNM_UTILS_H