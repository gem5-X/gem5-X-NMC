/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Functions to compute the convolution CnM kernel
 * input(hi*wi*ci) (x) co*weights(k*k*ci) + bias(co) = output(ho*wo*co)
 *
 */

#ifndef CNM_CONV_H
#define CNM_CONV_H

#include "cnm_utils.h"
#include "cnm_kernel.h"

class ConvolutionKernel : public Kernel {
    protected:
        // Convolution dimensions
        uint hi, wi, ci, k, ho, wo, co;
        // Convolution parameters
        uint stride, padding; //, dilation;
        bool relu;
        // Convolution inputs and output
        uint64_t* weights;
        uint64_t* bias;
        DataIdx input, output;

        uint parallelism;           // SIMD lanes per channel
        uint unrollLength;          // How many elements each unroll consists
        uint colsPerUnroll;         // How many columns each unroll consists of
        uint totalUnrolls;          // How many unrolls are needed to store the input
        uint totalColPerUnrrols;    // How many columns are needed to store the input

        std::vector<CnmCmd> crfFirstSeq, crfExtLoopSeq, crfExtPeelSeq;
        std::vector<CnmCmd> srfFirstSeq, srfExtLoopSeq, srfExtPeelSeq;

        int ext_loops, loops, ext_peeling;
        int crfSegment;

        void advanceInputTensor (uint* tensorRow, uint* tensorCol);
        void advanceWeightTensor (uint* tensorChannel, uint* tensorRow, uint* tensorCol);
        void weightAndLoop2Addr (uint* addrRow, uint* addrCol, uint weightChannel, uint weightRow, uint weightCol, uint loop);

        int allocateConvolutionKernel();
        int generateConvolutionKernelLimR();
        int generateConvolutionKernelLimC();
        int executeConvolutionKernelLimR();
        int executeConvolutionKernelLimC();

    public:
        ConvolutionKernel (CnmElements* _cnmElements, uint _channel,
                            uint _hi, uint _wi, uint _ci, uint _k, uint _co,
                            uint _stride, uint _padding, bool _relu = false);
        ~ConvolutionKernel();

        int generateSequence();
        int executeSequence();
        void storeKernel (uint64_t* dataA, uint64_t* dataB);
        void storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData);
        void loadResults (uint64_t* outputData);
};

ConvolutionKernel::ConvolutionKernel (CnmElements* _cnmElements, uint _channel,
                                        uint _hi, uint _wi, uint _ci, uint _k, uint _co,
                                        uint _stride, uint _padding, bool _relu) :
    Kernel(_cnmElements, KernelType::CONVOLUTION, _channel) {
        hi = _hi;
        wi = _wi;
        ci = _ci;
        k = _k;
        co = _co;
        stride = _stride;
        padding = _padding;
        //dilation = _dilation;
        relu = _relu;
        ho = ((hi + 2*padding - k) / stride) + 1;
        wo = ((wi + 2*padding - k) / stride) + 1;
        limC = CRF_ENTRIES < (SRF_M_ENTRIES + 4 + (relu ? 1 : 0));
        weights = NULL;
        bias = NULL;

        // Compute kernel auxiliar variables
        parallelism = SIMD_WIDTH * (NUM_BANK / 2) * NUM_BG;   // SIMD lanes per channel
        unrollLength = div_ceil(hi+2*padding-k+1, stride) * div_ceil(wi+2*padding-k+1, stride);    // How many elements each unroll consists
        colsPerUnroll = div_ceil(unrollLength, parallelism);   // How many columns each unroll consists of
        totalUnrolls = k * k * ci;   // How many unrolls are needed to store the input
        // How many columns are needed to store the input (half as even and odd banks are used for different unrolls, and rounded to the next even number)
        totalColPerUnrrols = 2*div_ceil(div_ceil(colsPerUnroll * totalUnrolls, 2), 2);
        if (allocateConvolutionKernel()) {
            std::cout << "Error allocating memory for the convolution kernel" << std::endl;
            exit(1);
        }  
        
        input.row = rowStart;
        input.col = 0;
        output.row = rowStart + totalColPerUnrrols / NUM_COL;
        output.col = totalColPerUnrrols % NUM_COL;
        addrStart = cnmExecAddress(channel, 0, 0, 0, rowStart, 0, cnmElements->execAddr);
#ifdef DEBUG
        std::cout << "Kernel is " << (limC ? "C-limited" : "R-limited") << std::endl;
#endif
}

ConvolutionKernel::~ConvolutionKernel() {
    // Find itself in cnmElements and delete the reference
    for (auto it = cnmElements->kernelList[channel].begin(); it != cnmElements->kernelList[channel].end(); it++) {
        if (*it == this) {
            cnmElements->kernelList[channel].erase(it);
            break;
        }
    }
    // Clear the sequence
    cnmSequence.clear();
    crfFirstSeq.clear();
    crfExtLoopSeq.clear();
    crfExtPeelSeq.clear();
    srfFirstSeq.clear();
    srfExtLoopSeq.clear();
    srfExtPeelSeq.clear();
}

int ConvolutionKernel::generateSequence() {
    if (limC) {
        return generateConvolutionKernelLimC();
    } else {
        return generateConvolutionKernelLimR();
    }
}

int ConvolutionKernel::executeSequence() {
    uint64_t dummyData = 0;
    int error;

    switchCnmMode(channel, &dummyData, cnmElements->rfAddr);    
    if (limC) {
        error = executeConvolutionKernelLimC();
    } else {
        error = executeConvolutionKernelLimR();
    }
    switchCnmMode(channel, &dummyData, cnmElements->rfAddr); 
    return error;
}

// Advance the input tensor indeces, in stride steps
void ConvolutionKernel::advanceInputTensor (uint* tensorRow, uint* tensorCol) {
    *tensorCol += stride;
    if (*tensorCol >= wi+2*padding) {
        *tensorCol = 0;
        *tensorRow += stride;
    }
}

// Advance the weight tensor indeces, in steps of 2 (even and odd banks)
void ConvolutionKernel::advanceWeightTensor (uint* tensorChannel, uint* tensorRow, uint* tensorCol) {
    *tensorCol += 2;  // Advance two columns as we are using even and odd banks
    if (*tensorCol >= k) {
        *tensorRow += *tensorCol / k;
        *tensorCol = *tensorCol % k;
        if (*tensorRow >= k) {
            *tensorChannel += *tensorRow / k;
            *tensorRow = *tensorRow % k;
        }
    }
}

// Compute the address of the unrolled input tensor data according to the weight and loop
void ConvolutionKernel::weightAndLoop2Addr (uint* addrRow, uint* addrCol, uint weightChannel, uint weightRow, uint weightCol, uint loop) {
    uint unrollIdx = weightChannel * k * k + weightRow * k + weightCol; // It belongs to the unroll of one of the weights
    uint absCol = (unrollIdx * colsPerUnroll) / 2 + loop;   // Absolute column in the unrolled input tensor; /2 as even and odd banks are used for different unrolls
    *addrRow = rowStart + absCol / NUM_COL;
    *addrCol = absCol % NUM_COL;
#if DEBUG
    if (weightRow >= k || weightCol >= k) {
        std::cout << "Error, kernel row or column out of bounds, processing unaligned activation" << std::endl;
        std::cout << std::dec << "weightChannel " << weightChannel << " weightRow " << weightRow << " weightCol " << weightCol << " loop " << loop << " unrollIdx " << unrollIdx << " absCol " << absCol << std::endl;
        exit(1);
    }
#endif
}

void ConvolutionKernel::storeKernel (uint64_t* dataA, uint64_t* dataB) {
    std::cout << "Error, storing convolution kernels needs three inputs: input, weights and bias";
    exit(1);
}

void ConvolutionKernel::storeKernel(uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData) {
    // We assume that the input and weights are stored in row major order
    uint64_t inputRepack[GRF_64B];
    uint colInIdx = input.col;
    uint rowInIdx = input.row;
    uint bankInIdx = 0;
    uint bgInIdx = 0;
    uint bankInParity = 0;

    uint tensorRow, tensorCol, tensorChannel;
    uint startTensorRow, startTensorCol, startTensorChannel;
    // uint endTensorRow, endTensorCol;
    uint endTensorCol;

    // Store the weights and bias
    weights = weightsData;
    bias = biasData;

    // Store the input
    startTensorCol = startTensorRow = startTensorChannel = 0;  // Indeces for the input tensor, not the DRAM address
    for (uint i = 0; i < totalUnrolls; i++) {
        // Store where the unroll starts
        tensorCol = startTensorCol;
        tensorRow = startTensorRow;
        tensorChannel = startTensorChannel;
        endTensorCol = wi + 2*padding - (k - startTensorCol - 1);
        // endTensorRow = hi + 2*padding - (k - startTensorRow - 1);
        for (uint j = 0; j < div_ceil(unrollLength, SIMD_WIDTH); j++) {
            for (uint l = 0; l < GRF_64B; l++) { 
                inputRepack[l] = 0;
                for (int m = 0; m < WORDS_PER_64B; m++) {
                    if (j*SIMD_WIDTH + l*WORDS_PER_64B + m < unrollLength) {    // Avoid out of bounds access
                        // Repack if not within padding, as otherwise is "storing" zeros
                        if (!(tensorRow < padding || tensorRow >= hi+padding || tensorCol < padding || tensorCol >= wi+padding)) {
                            uint idx = tensorChannel*div_ceil(hi*wi, SIMD_WIDTH)*SIMD_WIDTH + (tensorRow-padding)*wi + (tensorCol-padding);   // Channels are aligned to the DRAM column
                            // uint idx = tensorChannel*hi*wi + tensorRow*wi + tensorCol; 
                            inputRepack[l] |= ((inputData[idx / WORDS_PER_64B] >> (WORD_BITS*(idx % WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1)) << (WORD_BITS * m);
                        }
                        advanceInputTensor(&tensorRow, &tensorCol);
                        if (tensorCol >= endTensorCol) {
                            tensorCol = startTensorCol;
                            tensorRow += stride;
                        } else if (tensorCol < startTensorCol) {
                            tensorCol = startTensorCol;
                        }
                    }
                }
            }
#ifndef CHECKER
            memcpy(cnmExecAddress(channel, 0, bgInIdx, bankInIdx+bankInParity, rowInIdx, colInIdx, cnmElements->execAddr), inputRepack, GRF_64B*sizeof(uint64_t));
#else
            memcpy(addrStart + i*colsPerUnroll + j*GRF_64B, inputRepack, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Stored input unroll " << std::showbase <<  std::dec << i << " column " << j << " in input: ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgInIdx, bankInIdx+bankInParity, rowInIdx, colInIdx, cnmElements->execAddr) << " content: ";
            std::cout << std::hex << inputRepack[0] << " " << inputRepack[1] << " " << inputRepack[2] << " " << inputRepack[3];
            std::cout << std::dec << " bg " << bgInIdx << " bank " << bankInIdx+bankInParity << " row " << rowInIdx << " col " << colInIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgInIdx, &bankInIdx, &rowInIdx, &colInIdx);
        }
        // Start with aligned bg and bank, changing parity and column as needed (store rows in alternating bank parities)
        changeParityJumpBackCol(&bgInIdx, &bankInIdx, &rowInIdx, &colInIdx, &bankInParity, div_ceil(div_ceil(unrollLength, SIMD_WIDTH), (NUM_BG*NUM_BANK/2)));

        // Compute tensor row, col, ch for next unroll
        startTensorCol++;
        if (startTensorCol >= k) {
            startTensorCol = 0;
            startTensorRow++;
            if (startTensorRow >= k) {
                startTensorRow = 0;
                startTensorChannel++;
            }
        }
    }

    // Initialize the output to 0
    uint totalCol = div_ceil(ho*wo, SIMD_WIDTH);
    uint colOutIdx = output.col;
    uint rowOutIdx = output.row;
    uint bankOutIdx = 1;
    uint bgOutIdx = 0;
    uint64_t zero[GRF_64B] = {0};

    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < totalCol; j++) {
#ifndef CHECKER
            memcpy(cnmExecAddress(channel, 0, bgOutIdx, bankOutIdx, rowOutIdx, colOutIdx, cnmElements->execAddr), zero, GRF_64B*sizeof(uint64_t));
#else
            memcpy(addrStart + totalColPerUnrrols*GRF_64B + (i*totalCol+j)*GRF_64B, zero, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Pre-storing 0 in the output " << std::showbase << std::dec << (i*totalCol+j) << ": ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgOutIdx, bankOutIdx, rowOutIdx, colOutIdx, cnmElements->execAddr) << std::dec;
            std::cout << " bg " << bgOutIdx << " bank " << bankOutIdx << " row " << rowOutIdx << " col " << colOutIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgOutIdx, &bankOutIdx, &rowOutIdx, &colOutIdx); // All results come from GRF_B
        }
        if (bankOutIdx != 1 || bgOutIdx != 0) {  
            bgOutIdx = 0;
            bankOutIdx = 1;
            nextCol(&rowOutIdx, &colOutIdx);
        }
    }

#ifdef DEBUG
    std::cout << "Kernel data stored" << std::endl;
#endif
}

void ConvolutionKernel::loadResults(uint64_t* outputData) {
    uint totalCol = div_ceil(ho*wo, SIMD_WIDTH);
    uint colOutIdx = output.col;
    uint rowOutIdx = output.row;
    uint bankOutIdx = 1;
    uint bgOutIdx = 0;

    for (uint i = 0; i < co; i++) {
        for (uint j = 0; j < totalCol; j++) {
#ifndef CHECKER
            memcpy(outputData + (i*totalCol+j)*GRF_64B, cnmExecAddress(channel, 0, bgOutIdx, bankOutIdx, rowOutIdx, colOutIdx, cnmElements->execAddr), GRF_64B*sizeof(uint64_t));
#else
            memcpy(outputData + (i*totalCol+j)*GRF_64B, addrStart + totalColPerUnrrols*GRF_64B + (i*totalCol+j)*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Retrieving index " << std::showbase << std::dec << (i*totalCol+j) << " (idx by 64b " << (i*totalCol+j)*4 << ") in ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgOutIdx, bankOutIdx, rowOutIdx, colOutIdx, cnmElements->execAddr);
            std::cout << " contents " << std::hex << outputData[(i*totalCol+j)*2] << std::dec;
            std::cout << " bg " << bgOutIdx << " bank " << bankOutIdx << " row " << rowOutIdx << " col " << colOutIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgOutIdx, &bankOutIdx, &rowOutIdx, &colOutIdx); // All results come from GRF_B
        }
        if (bankOutIdx != 1 || bgOutIdx != 0) {  
            bgOutIdx = 0;
            bankOutIdx = 1;
            nextCol(&rowOutIdx, &colOutIdx);
        }
    }

#ifdef DEBUG
    std::cout << "Kernel output retrieved" << std::endl;
#endif
}

int ConvolutionKernel::allocateConvolutionKernel() {
    uint rows = div_ceil(totalColPerUnrrols + co*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG), NUM_COL);
    return allocateKernel(rows);
}



int ConvolutionKernel::generateConvolutionKernelLimR() {
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    ext_loops = (k*k*ci) / SRF_M_ENTRIES - 1;
    loops = div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    ext_peeling = (k*k*ci) % SRF_M_ENTRIES;

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! R-lim Conv. Loops = " << loops << std::endl;
    }

    // First set of weights, where bias is also added
    if (ext_loops > -1) {
        loopLen = SRF_M_ENTRIES + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0);
        crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAD, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 0, OPC_SRF_A, false));  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
        for (i = 0; i < SRF_M_ENTRIES/2; i++) {
            if (i)
                crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false));   // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (relu && !ext_peeling && ext_loops == 0) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));       // ReLU final result
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false));   // MOV to ODD_BANK
        } else {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false));   // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false)); // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false)); // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF first loop instructions written" << std::endl;
#endif

        // Write SRF contents of the first loop
        for (i = 0; i < SRF_M_ENTRIES; i++) {
            srfFirstSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
        srfFirstSeq.push_back(writeSRFA(0, rfAddr, 0));

#ifdef DEBUG
        std::cout << "SRF for first loop written" << std::endl;
#endif
    }

    // Next set of weights, until reaching ext_peeling
    if (ext_loops > 0) {
        crfIdx = 0;
        loopLen = SRF_M_ENTRIES + 2;
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial result)
        for (i = 0; i < SRF_M_ENTRIES/2; i++) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external loop instructions written" << std::endl;
#endif

        // Write SRF contents of the external loop
        for (i = 0; i < SRF_M_ENTRIES; i++) {
            srfExtLoopSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external loop written" << std::endl;
#endif
    }

    if (relu && ext_loops > 0 && !ext_peeling) {    // ReLU for the final results
        crfIdx = 0;
        loopLen = 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, true));  // ReLU to GRF_B (get current partial result)
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF for ReLU after external loop instructions written" << std::endl;
#endif
    }

    // Final set of weights (if ext_peeling)
    if (ext_peeling && ext_loops > 0) {
        crfIdx = ext_peeling + 1;
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU final result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    
        // Write SRF contents of the external peeling
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    } else if (ext_peeling && ext_loops > -1) {
        crfIdx = 0;
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial result)
        for (i = 0; i < ext_peeling/2; i++) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, ext_peeling-1, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
        }
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU final result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif

        // Write SRF contents of the external loop
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    } else if (ext_peeling) {
        int ext_peeling_internal = ext_peeling - 1;
        crfIdx = 0;
        loopLen = ext_peeling + 1 + (relu ? 1 : 0);
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAD, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 0, OPC_SRF_A, false));    // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
        for (i = 0; i < ext_peeling/2; i++) {
            if (i)
                crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false));     // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling_internal % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, ext_peeling-1, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
        }
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU partial result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif

        // Write SRF contents of the first loop
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
        srfExtPeelSeq.push_back(writeSRFA(0, rfAddr, 0));

#ifdef DEBUG
        std::cout << "SRF external peeling loop written" << std::endl;
#endif
    }
   
    return 0;
}

int ConvolutionKernel::executeConvolutionKernelLimR() {
    uint i;
    int j, l, m;
    uint loopLen;
    uint64_t* execAddr = cnmElements->execAddr;
    uint inCol = input.col;
    uint inRow = input.row;
    uint outCol = output.col;
    uint outRow = output.row;
    uint weightChannel, weightRow, weightCol;
    uint64_t dummyData = 0; // Dummy data to do ldrData and strData that trigger the execution

    if (ext_loops > -1) {
        // Commands to write CRF instructions for the first set of weights
        for (auto cmd : crfFirstSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF first loop instructions" << std::endl;
#endif
        // Commands to write SRF contents and trigger execution of the first set of weights
        loopLen = SRF_M_ENTRIES + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < SRF_M_ENTRIES; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfFirstSeq[j].addr, element);
            }
            strData(srfFirstSeq[SRF_M_ENTRIES].addr, bias[i / WORDS_PER_64B] >> (WORD_BITS*(i % WORDS_PER_64B)));
            // Trigger execution of the first set of weights
            for (j = 0; j < loops; j++) {
                weightCol = weightRow = weightChannel = 0;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
                for (l = 0; l < SRF_M_ENTRIES/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    if (l){
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    }
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);      // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (relu && !ext_peeling && ext_loops == 0) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU final result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0) < CRF_ENTRIES-1) {  // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0) < CRF_ENTRIES-1) {   // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered first set of weights for output channel " << i << std::endl;
#endif
        }
    }
    if (ext_loops > 0) {
        // Commands to write CRF instructions for the external loop
        for (auto cmd : crfExtLoopSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG    
        std::cout << "Writing CRF external loop instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external loop
        loopLen = SRF_M_ENTRIES + 2;
        for (i = 0; i < co; i++) {
            for (j = 0; j < ext_loops; j++) {
                // Write SRF contents
                for (l = 0; l < SRF_M_ENTRIES; l++) {
                    uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + (j+1)*SRF_M_ENTRIES + l;
                    uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                    strData(srfExtLoopSeq[l].addr, element);
                }
                // Trigger execution of the external loop
                for (l = 0; l < loops; l++) {
                    weightChannel = (j+1)*SRF_M_ENTRIES / (k*k);
                    weightRow = ((j+1)*SRF_M_ENTRIES % (k*k)) / k;
                    weightCol = ((j+1)*SRF_M_ENTRIES % (k*k)) % k;
                    uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + l;
                    outRow = output.row;
                    outCol = output.col;
                    jumpColAllBanks(&outRow, &outCol, outIdx);

                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // MOV to GRF_B (get current partial result)
                    for (m = 0; m < SRF_M_ENTRIES/2; m++) {
                        weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, l);
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                        ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += ODD_BANK * SRF_M
                        advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                    }
                    strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                    if (loops - 1) {    // Not only 1 iteration
                        ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                    }
                }
                if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
                } else if (loops && loopLen < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
                }
#if DEBUG
                std::cout << "Written SRF and triggered external loop for output channel " << i << std::endl;
#endif
            }
        }
    }

    if (relu && ext_loops > 0 && !ext_peeling) {
        // Commands to write CRF instructions for the ReLU after the external loop
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF for ReLU after external loop instructions" << std::endl;
#endif

        // Commands to trigger execution of ReLU after external loop
        loopLen = 2;
        for (i = 0; i < co; i++) {
            for (j = 0; j < loops; j++) {
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU to GRF_B (get current partial result)
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData);     // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if (loopLen + 1 < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
        }
#if DEBUG
        std::cout << "Triggered ReLU after external loop" << std::endl;
#endif
    }

    if (ext_peeling && ext_loops > -1) {
        // Commands to write CRF instructions for the external peeling
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external peeling
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + (ext_loops+1)*SRF_M_ENTRIES + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                weightChannel = (ext_loops+1)*SRF_M_ENTRIES / (k*k);
                weightRow = ((ext_loops+1)*SRF_M_ENTRIES % (k*k)) / k;
                weightCol = ((ext_loops+1)*SRF_M_ENTRIES % (k*k)) % k;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // MOV to GRF_B (get current partial result)
                for (l = 0; l < ext_peeling/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (ext_peeling % 2) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                }
                if (relu) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU partial result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + (relu ? 1 : 0) < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + (relu ? 1 : 0) < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered external peeling for output channel " << i << std::endl;
#endif
        }
    } else if (ext_peeling) {
        int ext_peeling_internal = ext_peeling - 1;
        // Commands to write CRF instructions for the external peeling
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external peeling
        loopLen = ext_peeling + 1 + (relu ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            strData(srfExtPeelSeq[ext_peeling].addr, bias[i / WORDS_PER_64B] >> (WORD_BITS*(i % WORDS_PER_64B)));
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                weightCol = weightRow = weightChannel = 0;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
                for (l = 0; l < ext_peeling/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    if (l)
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);      // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (ext_peeling_internal % 2) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                }
                if (relu) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU partial result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK

                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + (relu ? 1 : 0) < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + (relu ? 1 : 0) < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered external peeling for output channel " << i << std::endl;
#endif
        }
    }
    return 0;
}

int ConvolutionKernel::generateConvolutionKernelLimC() { 
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    crfSegment = ((CRF_ENTRIES - 4 - (relu ? 1 : 0)) / 2) * 2;   // Round down to even number
    crfSegment = std::min(crfSegment, SRF_M_ENTRIES);
    ext_loops = (k*k*ci) / crfSegment - 1;
    loops = div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    ext_peeling = (k*k*ci) % crfSegment;

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! C-lim Conv. Loops = " << loops << std::endl;
    }

    // First set of weights, where bias is also added
    if (ext_loops > -1) {
        loopLen = crfSegment + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0);
        crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAD, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 0, OPC_SRF_A, false));  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
        for (i = 0; i < crfSegment/2; i++) {
            if (i)
                crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false));       // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (relu && !ext_peeling && ext_loops == 0) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));       // ReLU final result
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false));   // MOV to ODD_BANK
        } else {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false));   // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false)); // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfFirstSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false)); // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF first loop instructions written" << std::endl;
#endif

        // Write SRF contents of the first loop
        for (i = 0; i < crfSegment; i++) {
            srfFirstSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
        srfFirstSeq.push_back(writeSRFA(0, rfAddr, 0));

#ifdef DEBUG
        std::cout << "SRF for first loop written" << std::endl;
#endif
    }

    // Next set of weghts, until reaching ext_peeling
    if (ext_loops > 0) {
        crfIdx = 0;
        loopLen = crfSegment + 2;
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial result)
        for (i = 0; i < crfSegment/2; i++) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external loop instructions written" << std::endl;
#endif

        // Write SRF contents of the external loop
        for (i = 0; i < crfSegment; i++) {
            srfExtLoopSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external loop written" << std::endl;
#endif
    }

    // ReLU after the external loop
    if (relu && ext_loops > 0 && !ext_peeling) {
        crfIdx = 0;
        loopLen = 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, true));  // ReLU to GRF_B (get current partial result)
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF for ReLU after external loop instructions written" << std::endl;
#endif
    }

    // Final set of weights (if ext_peeling)
    if (ext_peeling && ext_loops > 0) {
        crfIdx = ext_peeling + 1;
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU partial result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    
        // Write SRF contents of the external peeling
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    } else if (ext_peeling && ext_loops > -1) {
        crfIdx = 0;
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial result)
        for (i = 0; i < ext_peeling/2; i++) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, ext_peeling-1, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
        }
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU partial result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif

        // Write SRF contents of the external loop
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    } else if (ext_peeling) {
        int ext_peeling_internal = ext_peeling; // - 1;
        crfIdx = 0;
        loopLen = ext_peeling + 1 + (relu ? 1 : 0);
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAD, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 0, OPC_SRF_A, false));    // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
        for (i = 0; i < ext_peeling/2; i++) {
            if (i)
                crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false));     // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling_internal % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, ext_peeling-1, 0, false));    // MAC GRF_B += EVEN_BANK * SRF_M
        }
        if (relu) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 1, OPC_GRF_B, 0, 0, 0, 0, true));     // ReLU partial result
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 1, 0, 0, 0, false)); // MOV to ODD_BANK
        } else {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        }
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif

        // Write SRF contents of the first loop
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
        srfExtPeelSeq.push_back(writeSRFA(0, rfAddr, 0));

#ifdef DEBUG
        std::cout << "SRF external peeling loop written" << std::endl;
#endif
    }

    return 0;
}

int ConvolutionKernel::executeConvolutionKernelLimC() {
    uint i;
    int j, l, m;
    uint loopLen;
    uint64_t* execAddr = cnmElements->execAddr;
    uint inCol = input.col;
    uint inRow = input.row;
    uint outCol = output.col;
    uint outRow = output.row;
    uint weightChannel, weightRow, weightCol;
    uint64_t dummyData = 0; // Dummy data to do ldrData and strData that trigger the execution

    if (ext_loops > -1) {
        // Commands to write CRF instructions for the first set of weights
        for (auto cmd : crfFirstSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF first loop instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the first set of weights
        loopLen = crfSegment + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < crfSegment; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfFirstSeq[j].addr, element);
            }
            strData(srfFirstSeq[crfSegment].addr, bias[i / WORDS_PER_64B] >> (WORD_BITS*(i % WORDS_PER_64B)));
            // Trigger execution of the first set of weights
            for (j = 0; j < loops; j++) {
                weightCol = weightRow = weightChannel = 0;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
                for (l = 0; l < crfSegment/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    if (l){
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    }
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);      // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (relu && !ext_peeling && ext_loops == 0) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU partial result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0) < CRF_ENTRIES-1) {  // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + ((relu && !ext_peeling && ext_loops == 0) ? 1 : 0) < CRF_ENTRIES-1) {   // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered first set of weights for output channel " << i << std::endl;
#endif
        }
    }

    if (ext_loops > 0) {
        // Commands to write CRF instructions for the external loop
        for (auto cmd : crfExtLoopSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG    
        std::cout << "Writing CRF external loop instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external loop
        loopLen = crfSegment + 2;
        for (i = 0; i < co; i++) {
            for (j = 0; j < ext_loops; j++) {
                // Write SRF contents
                for (l = 0; l < crfSegment; l++) {
                    uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + (j+1)*crfSegment + l;
                    uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                    strData(srfExtLoopSeq[l].addr, element);
                }
                // Trigger execution of the external loop
                for (l = 0; l < loops; l++) {
                    weightChannel = (j+1)*crfSegment / (k*k);
                    weightRow = ((j+1)*crfSegment % (k*k)) / k;
                    weightCol = ((j+1)*crfSegment % (k*k)) % k;
                    uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + l;
                    outRow = output.row;
                    outCol = output.col;
                    jumpColAllBanks(&outRow, &outCol, outIdx);

                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // MOV to GRF_B (get current partial result)
                    for (m = 0; m < crfSegment/2; m++) {
                        weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, l);
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                        ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += ODD_BANK * SRF_M
                        advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                    }
                    strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                    if (loops - 1) {    // Not only 1 iteration
                        ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                    }
                }
                if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
                } else if (loops && loopLen < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
                }
#if DEBUG
                std::cout << "Written SRF and triggered external loop for output channel " << i << std::endl;
#endif
            }
        }
    }

    if (relu && ext_loops > 0 && !ext_peeling) {
        // Commands to write CRF instructions for the ReLU after the external loop
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF for ReLU after external loop instructions" << std::endl;
#endif

        // Commands to trigger execution of the ReLU after the external loop
        loopLen = 2;
        for (i = 0; i < co; i++) {
            for (j = 0; j < loops; j++) {
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU to GRF_B (get current partial result)
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData);     // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Triggered ReLU after external loop for output channel " << i << std::endl;
#endif
        }
    }

    if (ext_peeling && ext_loops > -1) {
        // Commands to write CRF instructions for the external peeling
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external peeling
        loopLen = ext_peeling + 2 + (relu ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + (ext_loops+1)*crfSegment + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                weightChannel = (ext_loops+1)*crfSegment / (k*k);
                weightRow = ((ext_loops+1)*crfSegment % (k*k)) / k;
                weightCol = ((ext_loops+1)*crfSegment % (k*k)) % k;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // MOV to GRF_B (get current partial result)
                for (l = 0; l < ext_peeling/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (ext_peeling % 2) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                }
                if (relu) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU partial result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + (relu ? 1 : 0) < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + (relu ? 1 : 0) < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered external peeling for output channel " << i << std::endl;
#endif
        }
    } else if (ext_peeling) {
        int ext_peeling_internal = ext_peeling;  // - 1;
        // Commands to write CRF instructions for the external peeling
        for (auto cmd : crfExtPeelSeq) {
            strData(cmd.addr, cmd.data);
        }
#ifdef DEBUG
        std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

        // Commands to write SRF contents and trigger execution of the external peeling
        loopLen = ext_peeling + 1 + (relu ? 1 : 0);
        for (i = 0; i < co; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint weightIdx = i*(div_ceil(k*k*ci, SIMD_WIDTH))*SIMD_WIDTH + j;
                uint64_t element = weights[weightIdx / WORDS_PER_64B] >> (WORD_BITS*(weightIdx % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            strData(srfExtPeelSeq[ext_peeling].addr, bias[i / WORDS_PER_64B] >> (WORD_BITS*(i % WORDS_PER_64B))); //To be tested - implemented the same way as R-lim
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                weightCol = weightRow = weightChannel = 0;
                uint outIdx = i*div_ceil(wo*ho, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) + j;
                outRow = output.row;
                outCol = output.col;
                jumpColAllBanks(&outRow, &outCol, outIdx);

                weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAD GRF_B = EVEN_BANK * SRF_M + SRF_A
                for (l = 0; l < ext_peeling/2; l++) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    if (l){
                        ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                    }
                    ldrData(cnmExecAddress(channel, 0, 0, 1, inRow, inCol, execAddr), &dummyData);      // MAC GRF_B += ODD_BANK * SRF_M
                    advanceWeightTensor(&weightChannel, &weightRow, &weightCol);
                }
                if (ext_peeling_internal % 2) {
                    weightAndLoop2Addr(&inRow, &inCol, weightChannel, weightRow, weightCol, j);
                    ldrData(cnmExecAddress(channel, 0, 0, 0, inRow, inCol, execAddr), &dummyData);  // MAC GRF_B += EVEN_BANK * SRF_M
                }
                if (relu) {
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // ReLU partial result
                }
                strData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), dummyData); // MOV to ODD_BANK

                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 + (relu ? 1 : 0) < CRF_ENTRIES-1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            } else if (loops && loopLen + (relu ? 1 : 0) < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, outRow, outCol, execAddr), &dummyData);    // EXIT
            }
#if DEBUG
            std::cout << "Written SRF and triggered external peeling for output channel " << i << std::endl;
#endif
        }
    }

    return 0;
}

#endif  // CNM_CONV_H