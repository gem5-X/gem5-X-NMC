/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * This file contains the library to use the Compute-near-Memory capabilities.
 *
 */

#ifndef CNM_H
#define CNM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// Includes that build the CnM library
#include "cnm_utils.h"
#include "cnm_intrinsics.h"
#include "cnm_kernel.h"
#include "cnm_va.h"
#include "cnm_dp.h"
#include "cnm_mm.h"
#include "cnm_conv.h"

// Function to map the RF and memory regions of the CnM DRAM
void cnmMemoryMap(CnmElements* cnmElements) {
#ifndef CHECKER
    cnmElements->rfAddr = memoryMap(LENGTH_MEM, RF_INST_MEM);
    cnmElements->execAddr  = memoryMap(LENGTH_MEM, EXEC_INST_MEM);
#else
    cnmElements->rfAddr = (uint64_t*)malloc(LENGTH_MEM);
    cnmElements->execAddr = (uint64_t*)malloc(LENGTH_MEM);
#endif

#ifdef DEBUG
    std::cout << "Memory mapped rfAddr at " << cnmElements->rfAddr << std::endl;
    std::cout << "Memory mapped execAddr at " << cnmElements->execAddr << std::endl;
#endif
}

// Function to unmap the RF and memory regions of the CnM DRAM
void cnmMemoryUnmap(CnmElements* cnmElements) {
#ifndef CHECKER
    memoryUnmap(cnmElements->rfAddr, LENGTH_MEM);
    memoryUnmap(cnmElements->execAddr, LENGTH_MEM);
#else
    free(cnmElements->rfAddr);
    free(cnmElements->execAddr);
#endif

#ifdef DEBUG
    std::cout << "Unmapped rfAddr and execAddr" << std::endl;
#endif
}

// Function to initialize a vector addition kernel configuration
Kernel* cnmInitVectorAdditionKernel(CnmElements* cnmElements, uint channel, uint numVectors, uint vectorDims) {
    VectorAdditionKernel* kernel = new VectorAdditionKernel(cnmElements, channel, numVectors, vectorDims);
    // Insert kernel into the kernel list ordered by rowStart
    auto it = cnmElements->kernelList[channel].begin();
    while (it != cnmElements->kernelList[channel].end() && (*it)->getRowStart() < kernel->getRowStart()) {
        it++;
    }
    cnmElements->kernelList[channel].insert(it, kernel);
#ifdef DEBUG
    std::cout << "Vector addition kernel initialized" << std::endl;
#endif
    return kernel;  // TODO check later if we want to return index instead
}

// Function to initialize a dot product kernel configuration
Kernel* cnmInitDotProductKernel(CnmElements* cnmElements, uint channel, uint numVectors, uint vectorDims) {
    DotProductKernel* kernel = new DotProductKernel(cnmElements, channel, numVectors, vectorDims);
    // Insert kernel into the kernel list ordered by rowStart
    auto it = cnmElements->kernelList[channel].begin();
    while (it != cnmElements->kernelList[channel].end() && (*it)->getRowStart() < kernel->getRowStart()) {
        it++;
    }
    cnmElements->kernelList[channel].insert(it, kernel);
#ifdef DEBUG
    std::cout << "Dot product kernel initialized" << std::endl;
#endif
    return kernel;  // TODO check later if we want to return index instead
}

// Function to initialize a matrix multiplication kernel configuration
Kernel* cnmInitMatrixMultiplicationKernel(CnmElements* cnmElements, uint channel, uint m, uint n, uint q) {
    MatrixMultiplicationKernel* kernel = new MatrixMultiplicationKernel(cnmElements, channel, m, n, q);
    // Insert kernel into the kernel list ordered by rowStart
    auto it = cnmElements->kernelList[channel].begin();
    while (it != cnmElements->kernelList[channel].end() && (*it)->getRowStart() < kernel->getRowStart()) {
        it++;
    }
    cnmElements->kernelList[channel].insert(it, kernel);
#ifdef DEBUG
    std::cout << "Matrix multiplication kernel initialized" << std::endl;
#endif
    return kernel;  // TODO check later if we want to return index instead
}

// Function to initialize a matrix multiplication kernel configuration
Kernel* cnmInitMatrixVectorMultiplicationKernel(CnmElements* cnmElements, uint channel, uint m, uint n) {
    MatrixMultiplicationKernel* kernel = new MatrixMultiplicationKernel(cnmElements, channel, 1, m, n);
    // Insert kernel into the kernel list ordered by rowStart
    auto it = cnmElements->kernelList[channel].begin();
    while (it != cnmElements->kernelList[channel].end() && (*it)->getRowStart() < kernel->getRowStart()) {
        it++;
    }
    cnmElements->kernelList[channel].insert(it, kernel);
#ifdef DEBUG
    std::cout << "Matrix multiplication kernel initialized" << std::endl;
#endif
    return kernel;  // TODO check later if we want to return index instead
}

// Function to initialize a convolution kernel configuration
Kernel* cnmInitConvolutionKernel(CnmElements* cnmElements, uint channel,
                                uint hi, uint wi, uint ci, uint k, uint co, uint stride, uint padding, bool relu) {
    ConvolutionKernel* kernel = new ConvolutionKernel(cnmElements, channel, hi, wi, ci, k, co, stride, padding, relu);
    // Insert kernel into the kernel list ordered by rowStart
    auto it = cnmElements->kernelList[channel].begin();
    while (it != cnmElements->kernelList[channel].end() && (*it)->getRowStart() < kernel->getRowStart()) {
        it++;
    }
    cnmElements->kernelList[channel].insert(it, kernel);
#ifdef DEBUG
    std::cout << "Convolution kernel initialized" << std::endl;
#endif
    return kernel;  // TODO check later if we want to return index instead
}

// Function to compute the kernel
int cnmComputeKernel(Kernel* kernel) {
    int error = 0;
    error = kernel->generateSequence();
    error = kernel->executeSequence();
    return error;
}

#endif  // CNM_H