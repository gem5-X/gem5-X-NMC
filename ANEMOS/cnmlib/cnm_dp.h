/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Functions to compute the dot product CnM kernel
 *
 */

#ifndef CNM_DP_H
#define CNM_DP_H

#include "cnm_utils.h"
#include "cnm_kernel.h"

class DotProductKernel : public Kernel {
    protected:
        uint numVectors;
        uint vectorDims;
        DataIdx vectorsA;
        DataIdx vectorsB;
        DataIdx results;

        int ext_loops, loops, peeling;
        int crfSegment;

        std::vector<CnmCmd> cnmLoopSeq, cnmPeelSeq;

        int allocateDotProductKernel();
        int generateDotProductKernelLimR();
        int generateDotProductKernelLimC();
        int executeDotProductKernelLimR();
        int executeDotProductKernelLimC();

    public:
        DotProductKernel(CnmElements* _cnmElements, uint _channel, uint _numVectors, uint _vectorDims);
        ~DotProductKernel();

        int generateSequence();
        int executeSequence();
        void storeKernel(uint64_t* vAdata, uint64_t* vBdata);
        void storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData);
        void loadResults(uint64_t* resData);
};

DotProductKernel::DotProductKernel(CnmElements* _cnmElements, uint _channel, uint _numVectors, uint _vectorDims) :
    Kernel(_cnmElements, KernelType::DOT_PRODUCT, _channel) {
        numVectors = _numVectors;
        vectorDims = _vectorDims;
        limC = CRF_ENTRIES < (4*(GRF_ENTRIES-1) + 2*(vectorDims % (2*(GRF_ENTRIES-1))) + 4);
        if (allocateDotProductKernel()) {
            std::cout << "Error allocating memory for the dot product kernel" << std::endl;
            exit(1);
        }
        vectorsA.row = rowStart;
        vectorsA.col = 0;
        vectorsB.row = rowStart + (div_ceil(vectorDims, 2)*div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG) / NUM_COL);
        vectorsB.col = (div_ceil(vectorDims, 2)*div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG)) % NUM_COL;
        results.row = rowStart + ((2*div_ceil(vectorDims, 2)*div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG)) / NUM_COL);
        results.col = (2*div_ceil(vectorDims, 2)*div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG)) % NUM_COL;
        addrStart = cnmExecAddress(channel, 0, 0, 0, rowStart, 0, cnmElements->execAddr);
#ifdef DEBUG
        std::cout << "Kernel is " << (limC ? "C-limited" : "R-limited") << std::endl;
#endif
}

DotProductKernel::~DotProductKernel() {
    // Find itself in cnmElements and delete the reference
    for (auto it = cnmElements->kernelList[channel].begin(); it != cnmElements->kernelList[channel].end(); it++) {
        if (*it == this) {
            cnmElements->kernelList[channel].erase(it);
            break;
        }
    }
    // Clear the sequence
    cnmSequence.clear();
    cnmLoopSeq.clear();
    cnmPeelSeq.clear();
}

int DotProductKernel::generateSequence() {
    if (limC) {
        return generateDotProductKernelLimC();
    } else {
        return generateDotProductKernelLimR();
    }
}

int DotProductKernel::executeSequence() {
    uint64_t dummyData = 0;
    int error;

    switchCnmMode(channel, &dummyData, cnmElements->rfAddr);    
    if (limC) {
        error = executeDotProductKernelLimC();
    } else {
        error = executeDotProductKernelLimR();
    }
    switchCnmMode(channel, &dummyData, cnmElements->rfAddr); 
    return error;   
}

void DotProductKernel::storeKernel(uint64_t* vAdata, uint64_t* vBdata) {
    uint64_t intlvA[GRF_64B], intlvB[GRF_64B];    // To repackage 4 64-bit values into 4 64-bit values interleaved
    uint maxIdx = div_ceil(numVectors*vectorDims, SIMD_WIDTH) * GRF_64B;
    uint colAIdx = vectorsA.col;
    uint rowAIdx = vectorsA.row;
    uint bankAIdx = 0;
    uint bgAIdx = 0;
    uint bankAParity = 0;
    uint colBIdx = vectorsB.col;
    uint rowBIdx = vectorsB.row;
    uint bankBIdx = 0;
    uint bgBIdx = 0;
    uint bankBParity = 0;

    // Store in SIMD_WIDTH-element blocks to exploit cache locality
    for (uint i = 0; i < numVectors; i += SIMD_WIDTH) {
        for (uint j = 0; j < vectorDims; j++) {
            for (uint k = 0; k < GRF_64B; k++) {   // x 64-bit values per GRF_BITS column
                intlvA[k] = intlvB[k] = 0;
                for (uint l = 0; l < WORDS_PER_64B; l++) {
                    uint idx = ((i+k*WORDS_PER_64B+l)*vectorDims+j)/WORDS_PER_64B;
                    if (idx < maxIdx) { // Avoid out-of-bounds access
                        intlvA[k] |= ((vAdata[idx] >> (WORD_BITS*((j+l*vectorDims)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1)) << (WORD_BITS * l);
                        intlvB[k] |= ((vBdata[idx] >> (WORD_BITS*((j+l*vectorDims)%WORDS_PER_64B))) & ((1UL << WORD_BITS) - 1)) << (WORD_BITS * l);
#ifdef DEBUG                        
                        std::cout << "vector chunk " << i << ", dimension chunk " << j << ", k " << k << ", l " << l << std::endl;
                        std::cout << "vAdata[" << i*vectorDims/WORDS_PER_64B + j/WORDS_PER_64B + (k*WORDS_PER_64B+l)*vectorDims/WORDS_PER_64B << "] = " << std::hex << vAdata[i*vectorDims/WORDS_PER_64B + j/WORDS_PER_64B + (k*WORDS_PER_64B+l)*vectorDims/WORDS_PER_64B] << std::dec << std::endl;
                        std::cout << "intlvA[" << k << "] = " << std::hex << intlvA[k] << std::dec << std::endl;
                        std::cout << "vBdata[" << i*vectorDims/WORDS_PER_64B + j/WORDS_PER_64B + (k*WORDS_PER_64B+l)*vectorDims/WORDS_PER_64B << "] = " << std::hex << vBdata[i*vectorDims/WORDS_PER_64B + j/WORDS_PER_64B + (k*WORDS_PER_64B+l)*vectorDims/WORDS_PER_64B] << std::dec << std::endl;
                        std::cout << "intlvB[" << k << "] = " << std::hex << intlvB[k] << std::dec << std::endl;
#endif                    
                    }
                }
            }
#ifndef CHECKER
            memcpy(cnmExecAddress(channel, 0, bgAIdx, bankAIdx+bankAParity, rowAIdx, colAIdx, cnmElements->execAddr), intlvA, GRF_64B*sizeof(uint64_t));
            memcpy(cnmExecAddress(channel, 0, bgBIdx, bankBIdx+bankBParity, rowBIdx, colBIdx, cnmElements->execAddr), intlvB, GRF_64B*sizeof(uint64_t));
#else   
            // std::cout << "Storing to addresses " << std::hex << cnmExecAddress(channel, 0, bgAIdx, bankAIdx+bankAParity, rowAIdx, colAIdx, cnmElements->execAddr) << " and ";
            // std::cout << cnmExecAddress(channel, 0, bgBIdx, bankBIdx+bankBParity, rowBIdx, colBIdx, cnmElements->execAddr) << std::dec << std::endl;
            memcpy(addrStart + i*GRF_64B, intlvA, GRF_64B*sizeof(uint64_t));
            memcpy(addrStart + numVectors*vectorDims/WORDS_PER_64B + i*GRF_64B, intlvB + i*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Storing vector chunk " << std::showbase <<  std::dec <<  i << ", dimension chunk " << j << " in A: ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgAIdx, bankAIdx+bankAParity, rowAIdx, colAIdx, cnmElements->execAddr) << " content: ";
            std::cout << std::hex << intlvA[0] << " " << intlvA[1] << " " << intlvA[2] << " " << intlvA[3];
            std::cout << std::dec << " bg " << bgAIdx << " bank " << bankAIdx+bankAParity << " row " << rowAIdx << " col " << colAIdx;
            std::cout << ";\tand in B: ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgBIdx, bankBIdx+bankBParity, rowBIdx, colBIdx, cnmElements->execAddr) << " content: ";
            std::cout << std::hex << intlvB[0] << " " << intlvB[1] << " " << intlvB[2] << " " << intlvB[3];
            std::cout << std::dec << " bg " << bgBIdx << " bank " << bankBIdx+bankBParity << " row " << rowBIdx << " col " << colBIdx << std::endl;
#endif
            // Advance column, as dimensions for the same vector should be in the same bank, different columns
            nextColChangeParity(&rowAIdx, &colAIdx, &bankAParity);  // Leverage the even and odd banks, then jump column
            nextColChangeParity(&rowBIdx, &colBIdx, &bankBParity);
        }
        nextBgBankJumpBackColKeepParity(&bgAIdx, &bankAIdx, &rowAIdx, &colAIdx, bankAParity, vectorDims/2);
        nextBgBankJumpBackColKeepParity(&bgBIdx, &bankBIdx, &rowBIdx, &colBIdx, bankBParity, vectorDims/2);
        bankAParity = bankBParity = 0;
    }

    // Initialice the results to 0
    uint totalCol = div_ceil(numVectors, SIMD_WIDTH);
    uint colIdx = results.col;
    uint rowIdx = results.row;
    uint bankIdx = 1;
    uint bgIdx = 0;
    uint64_t zero[GRF_64B] = {0};

    for (uint i = 0; i < totalCol; i++) {
#ifndef CHECKER
        memcpy(cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr), zero, GRF_64B*sizeof(uint64_t));
#else
        memcpy(addrStart + 2*numVectors*vectorDims/WORDS_PER_64B + i*GRF_64B, zero, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
        std::cout << "Pre-storing 0 in the result " << std::showbase <<  std::dec <<  i << ": ";
        std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr) << std::dec;
        std::cout << std::dec << " bg " << bgIdx << " bank " << bankIdx << " row " << rowIdx << " col " << colIdx << std::endl;
#endif
        nextColChunkBgBaKeepBankParity(&bgIdx, &bankIdx, &rowIdx, &colIdx); // All results come from GRF_B
    }

#ifdef DEBUG
    std::cout << "Kernel data stored" << std::endl;
#endif
}

void DotProductKernel::storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData) {
    std::cout << "Error, storing convolution kernels needs two inputs: V1 and V2";
    exit(1);
}

void DotProductKernel::loadResults(uint64_t* resData) {
    uint totalCol = div_ceil(numVectors, SIMD_WIDTH);
    uint colIdx = results.col;
    uint rowIdx = results.row;
    uint bankIdx = 1;
    uint bgIdx = 0;

    // Store the vectors in column chunks, jumping over channel and offset bits
    for (uint i = 0; i < totalCol; i++) {
#ifndef CHECKER
        memcpy(resData + i*GRF_64B, cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr), GRF_64B*sizeof(uint64_t));
#else
        memcpy(resData + i*GRF_64B, addrStart + 2*numVectors*vectorDims/WORDS_PER_64B + i*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
        std::cout << "Retrieving index " << std::dec <<  i << " in ";
        std::cout << " address " << std::hex << cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr) << " content: ";
        std::cout << std::hex << resData[(i+0)*GRF_64B] << " " << resData[(i+1)*GRF_64B] << " " << resData[(i+2)*GRF_64B] << " " << resData[(i+3)*GRF_64B];
        std::cout << std::dec << " bg " << bgIdx << " bank " << bankIdx << " row " << rowIdx << " col " << colIdx << std::endl;
#endif
        nextColChunkBgBaKeepBankParity(&bgIdx, &bankIdx, &rowIdx, &colIdx);
    }
#ifdef DEBUG
    std::cout << "Kernel results retrieved" << std::endl;
#endif
}

int DotProductKernel::allocateDotProductKernel() {
    uint rows = div_ceil((2*vectorDims+1)*div_ceil(numVectors, SIMD_WIDTH*NUM_BANK*NUM_BG), NUM_COL);   // Better space optimization
    return allocateKernel(rows);
}

int DotProductKernel::generateDotProductKernelLimR() {
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    ext_loops = div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    loops = vectorDims / (2*(GRF_ENTRIES-1));
    peeling = vectorDims % (2*(GRF_ENTRIES-1));

    loopLen = loops ? 4*(GRF_ENTRIES-1) : 0;

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! R-lim DP. Loops = " << loops << std::endl;
    }

    // Write instructions to the CRF for the loop
    cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, GRF_ENTRIES-1, OPC_ODD_BANK, 0, 0, 0, 0, false));   // MOV to GRF_B (initialize to 0)
    if (loops) {
        for (i = 0; i < GRF_ENTRIES-1; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));   // MOV to GRF_B
        }
        for (i = 0; i < GRF_ENTRIES-1; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // MAC GRF_B += GRF_A * EVEN_BANK
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));   // MAC GRF_B += GRF_B * ODD_BANK
        }
        if (loops - 1) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false)); // JUMP to start
        }
    }
    if (peeling) {
        for (i = 0; i < peeling/2; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));   // MOV to GRF_B
        }
        if (peeling % 2) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // MAC GRF_B += GRF_A * EVEN_BANK
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));   // MAC GRF_B += GRF_B * ODD_BANK
        }
        if (peeling % 2) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // MAC GRF_B += GRF_A * EVEN_BANK
        }
    }
    cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, GRF_ENTRIES-1, 0, 0, 0, false));   // MOV to ODD_BANK
    if (crfIdx < CRF_ENTRIES) {
        cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false)); // EXIT
    }
#ifdef DEBUG
        std::cout << "CRF instructions written" << std::endl;
#endif
    
    return 0;
}

int DotProductKernel::executeDotProductKernelLimR() {
    int i, j, k;
    uint loopLen = loops ? 4*(GRF_ENTRIES-1) : 0;
    uint64_t* execAddr = cnmElements->execAddr;
    uint vACol = vectorsA.col;
    uint vARow = vectorsA.row;
    uint vBCol = vectorsB.col;
    uint vBRow = vectorsB.row;
    uint resCol = results.col;
    uint resRow = results.row;
    uint64_t dummyData = 0;

    // Commands to write CRF instructions for the kernel
    for (auto cmd : cnmSequence) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF instructions" << std::endl;
#endif
    
    // Commands to trigger execution of the kernel
    for (i = 0; i < ext_loops; i++) {
        ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData); // MOV to GRF_B (initialize to 0
        for (j = 0; j < loops; j++) {
            for (k = 0; k < GRF_ENTRIES-1; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);  // MOV to GRF_A
                ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);  // MOV to GRF_B
                nextColAllBanks(&vARow, &vACol);
            }
            for (k = 0; k < GRF_ENTRIES-1; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);  // MAC GRF_B += GRF_A * EVEN_BANK
                ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);  // MAC GRF_B += GRF_B * ODD_BANK
                nextColAllBanks(&vBRow, &vBCol);
            }
            if (loops - 1) {    // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);  // JUMP to start
            }
        }
        if (peeling) {
            for (k = 0; k < peeling/2; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);  // MOV to GRF_A
                ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);  // MOV to GRF_B
                nextColAllBanks(&vARow, &vACol);
            }
            if (peeling % 2) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
                nextColAllBanks(&vARow, &vACol);
            }
            for (k = 0; k < peeling/2; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);  // MAC GRF_B += GRF_A * EVEN_BANK
                ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);  // MAC GRF_B += GRF_B * ODD_BANK
                nextColAllBanks(&vBRow, &vBCol);
            }
            if (peeling % 2) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);  // MAC GRF_B += GRF_A * EVEN_BANK
                nextColAllBanks(&vBRow, &vBCol);
            }
        }
        strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData); // MOV to ODD_BANK
        if (1 + loopLen + 1 + peeling*2 + 1 < CRF_ENTRIES-1) {
            ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);    // EXIT
        }
        nextColAllBanks(&resRow, &resCol);  // Advance after EXIT to use same address
    }
#ifdef DEBUG
    std::cout << "Triggered kernel execution" << std::endl;
#endif

    return 0;
}

int DotProductKernel::generateDotProductKernelLimC() {
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    crfSegment = ((CRF_ENTRIES-4)/4) * 2;   // Round to a multiple of 2
    crfSegment = std::min(crfSegment, int(2*(GRF_ENTRIES-1)));
    ext_loops = div_ceil(numVectors, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    loops = vectorDims / (crfSegment);
    peeling = vectorDims % (crfSegment);

    loopLen = loops ? 2*crfSegment : 0; // MOVs and MACs for the GRFs

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! C-lim DP. Loops = " << loops << std::endl;
    }

    // Write instructions to the CRF for the loop
    if (loops) {
        cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, GRF_ENTRIES-1, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B (initialize to 0)
        for (i = 0; i < crfSegment/2; i++) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B
        }
        for (i = 0; i < crfSegment/2; i++) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // MAC GRF_B += GRF_A * EVEN_BANK
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));    // MAC GRF_B += GRF_B * ODD_BANK
        }
        if (loops - 1) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));  // JUMP to start 
        }
        cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, GRF_ENTRIES-1, 0, 0, 0, false));    // MOV to ODD_BANK
        if (crfIdx < CRF_ENTRIES) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF loop instructions written" << std::endl;
#endif
    }
    
    // Write instructions to the CRF for the peeled part
    if (peeling) {
        crfIdx = 0;
        cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, GRF_ENTRIES-1, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B (initialize to 0)
        for (i = 0; i < peeling/2; i++) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B
        }
        if (peeling % 2) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // MAC GRF_B += GRF_A * EVEN_BANK
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));    // MAC GRF_B += GRF_B * ODD_BANK
        }
        if (peeling % 2) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, GRF_ENTRIES-1, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // MAC GRF_B += GRF_A * EVEN_BANK
        }
        cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, GRF_ENTRIES-1, 0, 0, 0, false));    // MOV to ODD_BANK
        if (crfIdx < CRF_ENTRIES) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF peeling instructions written" << std::endl;
#endif
    }

    return 0;
}

int DotProductKernel::executeDotProductKernelLimC() {
    int i, j, k;
    uint loopLen = loops ? 2*crfSegment : 0;
    uint64_t* execAddr = cnmElements->execAddr;
    uint vACol = vectorsA.col;
    uint vARow = vectorsA.row;
    uint vBCol = vectorsB.col;
    uint vBRow = vectorsB.row;
    uint resCol = results.col;
    uint resRow = results.row;
    uint64_t dummyData = 0;

    // Commands to write CRF loop instructions for the kernel
    for (auto cmd : cnmLoopSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF loop instructions" << std::endl;
#endif
    
    // Commands to trigger loop execution of the kernel
    if (loops) {
        for (i = 0; i < ext_loops; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);  // MOV to GRF_B (initialize to 0
            for (j = 0; j < loops; j++) {
                for (k = 0; k < crfSegment/2; k++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
                    nextColAllBanks(&vARow, &vACol);
                }
                for (k = 0; k < crfSegment/2; k++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // MAC GRF_B += GRF_A * EVEN_BANK
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // MAC GRF_B += GRF_B * ODD_BANK
                    nextColAllBanks(&vBRow, &vBCol);
                }
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // JUMP to start
                }
            }
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
            if ((loops-1) > 0 && 1 + loopLen + 1 + 1 < CRF_ENTRIES-1) { // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);  // EXIT
            } else if (loops && 1 + loopLen + 1 < CRF_ENTRIES-1) { // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);  // EXIT
            }
            nextColAllBanks(&resRow, &resCol);  // Advance after EXIT to use same address
            if (peeling) {
                // Jump over addresses used for the peeled part
                jumpColAllBanks(&vARow, &vACol, div_ceil(peeling, 2));
                jumpColAllBanks(&vBRow, &vBCol, div_ceil(peeling, 2));
            }
        }
#ifdef DEBUG
        std::cout << "Triggered loop execution of kernel" << std::endl;
#endif
    }

    // Commands to write CRF peeling instructions for the kernel
    for (auto cmd : cnmPeelSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF peeling instructions" << std::endl;   
#endif

    // Commands to trigger peeling execution of the kernel
    vACol = vectorsA.col;
    vARow = vectorsA.row;
    vBCol = vectorsB.col;
    vBRow = vectorsB.row;
    jumpColAllBanks(&vARow, &vACol, loops*crfSegment/2);
    jumpColAllBanks(&vBRow, &vBCol, loops*crfSegment/2);
    resCol = results.col;
    resRow = results.row;
    if (peeling) {
        for (i = 0; i < ext_loops; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);  // MOV to GRF_B (initialize to 0
            for (k = 0; k < peeling/2; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
                ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
                nextColAllBanks(&vARow, &vACol);
            }
            if (peeling % 2) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
                nextColAllBanks(&vARow, &vACol);
            }
            for (k = 0; k < peeling/2; k++) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // MAC GRF_B += GRF_A * EVEN_BANK
                ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // MAC GRF_B += GRF_B * ODD_BANK
                nextColAllBanks(&vBRow, &vBCol);
            }
            if (peeling % 2) {
                ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // MAC GRF_B += GRF_A * EVEN_BANK
                nextColAllBanks(&vBRow, &vBCol);
            }
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
            if (1 + peeling*2 + 1 < CRF_ENTRIES-1) {
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);  // EXIT
            }
            nextColAllBanks(&resRow, &resCol);  // Advance after EXIT to use same address
            if (loops) {
                // Jump to the next addresses used for the peeled part
                jumpColAllBanks(&vARow, &vACol, loops*crfSegment/2);
                jumpColAllBanks(&vBRow, &vBCol, loops*crfSegment/2);
            }
        }
#ifdef DEBUG
        std::cout << "Triggered peeling execution of kernel" << std::endl;
#endif
    }

    return 0;
}

#endif // CNM_DP_H