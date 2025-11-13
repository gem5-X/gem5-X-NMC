/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Functions to compute the vector addition CnM kernel
 *
 */

#ifndef CNM_VA_H
#define CNM_VA_H

#include "cnm_utils.h"
#include "cnm_kernel.h"

class VectorAdditionKernel : public Kernel {
    protected:
        uint numVectors;
        uint vectorDims;
        DataIdx vectorsA;
        DataIdx vectorsB;
        DataIdx results;

        int loops, peeling, extraLoops;
        int crfSegment;

        std::vector<CnmCmd> cnmLoopSeq, cnmPeelSeq, cnmExtraLoopSeq;

        int allocateVectorAdditionKernel();
        int generateVectorAdditionKernelLimR();
        int generateVectorAdditionKernelLimC();
        int executeVectorAdditionKernelLimR();
        int executeVectorAdditionKernelLimC();

    public:
        VectorAdditionKernel(CnmElements* _cnmElements, uint _channel, uint _numVectors, uint _vectorDims);
        ~VectorAdditionKernel();

        int generateSequence();
        int executeSequence();
        void storeKernel(uint64_t* vAdata, uint64_t* vBdata);
        void storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData);
        void loadResults(uint64_t* resData);
};


VectorAdditionKernel::VectorAdditionKernel(CnmElements* _cnmElements, uint _channel, uint _numVectors, uint _vectorDims) :
    Kernel(_cnmElements, KernelType::VECTOR_ADDITION, _channel) {
        numVectors = _numVectors;
        vectorDims = _vectorDims;
        limC = CRF_ENTRIES < (6*GRF_ENTRIES + 3*(div_ceil(numVectors*vectorDims, SIMD_WIDTH*CORES_PER_PCH) * 2*GRF_ENTRIES) + 2);
        if (allocateVectorAdditionKernel()) {
            std::cout << "Error allocating memory for the vector addition kernel" << std::endl;
            exit(1);
        }
        vectorsA.row = rowStart;
        vectorsA.col = 0;
        vectorsB.row = rowStart + (div_ceil(numVectors*vectorDims, SIMD_WIDTH*NUM_BANK*NUM_BG) / NUM_COL);
        vectorsB.col = div_ceil(numVectors*vectorDims, SIMD_WIDTH*NUM_BANK*NUM_BG) % NUM_COL;
        results.row = rowStart + (2*div_ceil(numVectors*vectorDims, SIMD_WIDTH*NUM_BANK*NUM_BG) / NUM_COL);
        results.col = 2*div_ceil(numVectors*vectorDims, SIMD_WIDTH*NUM_BANK*NUM_BG) % NUM_COL;
        addrStart = cnmExecAddress(channel, 0, 0, 0, rowStart, 0, cnmElements->execAddr);
// #ifdef DEBUG
        std::cout << "Kernel is " << (limC ? "C-limited" : "R-limited") << std::endl;
// #endif
}

VectorAdditionKernel::~VectorAdditionKernel() {
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

int VectorAdditionKernel::generateSequence() {
    if (limC) {
        return generateVectorAdditionKernelLimC();
    } else {
        return generateVectorAdditionKernelLimR();
    }
}

int VectorAdditionKernel::executeSequence() {
    uint64_t dummyData = 0;
    int error;

    switchCnmMode(channel, &dummyData, cnmElements->rfAddr);    
    if (limC) {
        error = executeVectorAdditionKernelLimC();
    } else {
        error = executeVectorAdditionKernelLimR();
    }
    switchCnmMode(channel, &dummyData, cnmElements->rfAddr); 
    return error;
}

void VectorAdditionKernel::storeKernel(uint64_t* vAdata, uint64_t* vBdata) {
    uint numElements = numVectors * vectorDims;
    uint totalCol = div_ceil(numElements, SIMD_WIDTH);
    uint colAIdx = vectorsA.col;
    uint rowAIdx = vectorsA.row;
    uint bankAIdx = 0;
    uint bgAIdx = 0;
    uint colBIdx = vectorsB.col;
    uint rowBIdx = vectorsB.row;
    uint bankBIdx = 0;
    uint bgBIdx = 0;
    
    // Store the vectors in column chunks, jumping over channel and offset bits
    for (uint i = 0; i < totalCol; i++) {
#ifndef CHECKER
        memcpy(cnmExecAddress(channel, 0, bgAIdx, bankAIdx, rowAIdx, colAIdx, cnmElements->execAddr), vAdata + i*GRF_64B, GRF_64B*sizeof(uint64_t));
        memcpy(cnmExecAddress(channel, 0, bgBIdx, bankBIdx, rowBIdx, colBIdx, cnmElements->execAddr), vBdata + i*GRF_64B, GRF_64B*sizeof(uint64_t));
#else
        memcpy(addrStart + i*GRF_64B, vAdata + i*GRF_64B, GRF_64B*sizeof(uint64_t));
        memcpy(addrStart + div_ceil(numVectors*vectorDims, WORDS_PER_64B) + i*GRF_64B, vBdata + i*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
        std::cout << "Storing index " << std::showbase <<  std::dec <<  i << " in A: ";
        std::cout << std::hex << vAdata[i*GRF_64B];
        std::cout << std::dec << " bg " << bgAIdx << " bank " << bankAIdx << " row " << rowAIdx << " col " << colAIdx;
        std::cout << ";\tand in B: ";
        std::cout << std::hex << vBdata[i*GRF_64B];
        std::cout << std::dec << " bg " << bgBIdx << " bank " << bankBIdx << " row " << rowBIdx << " col " << colBIdx << std::endl;
#endif
        nextColChunkBgBa(&bgAIdx, &bankAIdx, &rowAIdx, &colAIdx);
        nextColChunkBgBa(&bgBIdx, &bankBIdx, &rowBIdx, &colBIdx);
    }
#ifdef DEBUG
    std::cout << "Kernel data stored" << std::endl;
#endif
}

void VectorAdditionKernel::storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData) {
    std::cout << "Error, storing convolution kernels needs two inputs: V1 and V2";
    exit(1);
}

void VectorAdditionKernel::loadResults(uint64_t* resData) {
    uint numElements = numVectors * vectorDims;
    uint totalCol = div_ceil(numElements, SIMD_WIDTH);
    uint colIdx = results.col;
    uint rowIdx = results.row;
    uint bankIdx = 0;
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
        std::cout << std::hex << resData[i*GRF_64B];
        std::cout << std::dec << " bg " << bgIdx << " bank " << bankIdx << " row " << rowIdx << " col " << colIdx << std::endl;
#endif
        nextColChunkBgBa(&bgIdx, &bankIdx, &rowIdx, &colIdx);
    }
#ifdef DEBUG
    std::cout << "Kernel results retrieved" << std::endl;
#endif
}

int VectorAdditionKernel::allocateVectorAdditionKernel() {
    // Calculate the number of rows needed for the kernel
    uint rows = div_ceil(3*div_ceil(numVectors*vectorDims, SIMD_WIDTH*NUM_BANK*NUM_BG), NUM_COL);
    return allocateKernel(rows);
}

int VectorAdditionKernel::generateVectorAdditionKernelLimR() {
    int i;
    uint loopLen = loops ? 6*GRF_ENTRIES : 0;   // MOVs, ADDs and MOVs for the 2 GRFs
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    loops = div_ceil(numVectors*vectorDims, SIMD_WIDTH*CORES_PER_PCH) / (2*GRF_ENTRIES);
    peeling = div_ceil(numVectors*vectorDims, SIMD_WIDTH*CORES_PER_PCH) % (2*GRF_ENTRIES);

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! R-lim VA. Loops = " << loops << std::endl;
    }

    // Write instructions to the CRF for the loop
    if (loops) {
        for (i = 0; i < GRF_ENTRIES; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));   // MOV to GRF_B
        }
        for (i = 0; i < GRF_ENTRIES; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // ADD GRF_A = GRF_A + EVEN_BANK
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_B, i, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));   // ADD GRF_B = GRF_B + ODD_BANK
        }
        for (i = 0; i < GRF_ENTRIES; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));  // MOV from GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, i, 0, 0, 0, false));   // MOV from GRF_B
        }
        if (loops - 1) {    // Not only 1 iteration
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false)); // JUMP to start
        }
#ifdef DEBUG
        std::cout << "CRF loop instructions written" << std::endl;
#endif
    }

    // Write instructions to the CRF for the peeled part
    if (peeling) {
        for (i = 0; i < peeling/2; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));   // MOV to GRF_B
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));  // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // ADD GRF_A = GRF_A + EVEN_BANK
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_B, i, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));   // ADD GRF_B = GRF_B + ODD_BANK
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));  // ADD GRF_A = GRF_A + EVEN_BANK
        }
        for (i = 0; i < peeling/2; i++) {
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));  // MOV from GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, i, 0, 0, 0, false));   // MOV from GRF_B
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));  // MOV from GRF_A
        }
#ifdef DEBUG
        std::cout << "CRF peeling instructions written" << std::endl;
#endif
    }

    if (crfIdx < CRF_ENTRIES) {
        cnmSequence.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false)); // EXIT
    }

    return 0;
}

int VectorAdditionKernel::executeVectorAdditionKernelLimR() {
    int i, j;
    uint loopLen = loops ? 6*GRF_ENTRIES : 0;   // MOVs, ADDs and MOVs for the 2 GRFs
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

    // Commands to trigger execution of the kernel loop
    for (i = 0; i < loops; i++) {
        for (j = 0; j < GRF_ENTRIES; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
            nextColAllBanks(&vARow, &vACol);
        }
        for (j = 0; j < GRF_ENTRIES; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
            ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_B = GRF_B + ODD_BANK
            nextColAllBanks(&vBRow, &vBCol);
        }
        for (j = 0; j < GRF_ENTRIES; j++) {
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);    // MOV from GRF_A
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);    // MOV from GRF_B
            nextColAllBanks(&resRow, &resCol);
        }
        if (loops - 1) {    // Not only 1 iteration
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);  // JUMP to start
        }  
    }
#ifdef DEBUG
    std::cout << "Triggered kernel loop execution" << std::endl;
#endif

    if (peeling) {
        for (i = 0; i < peeling/2; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
            nextColAllBanks(&vARow, &vACol);
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
            ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_B = GRF_B + ODD_BANK
            nextColAllBanks(&vBRow, &vBCol);
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
        }
        for (i = 0; i < peeling/2; i++) {
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);    // MOV from GRF_A
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);    // MOV from GRF_B
            nextColAllBanks(&resRow, &resCol);
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);    // MOV from GRF_A
        }
    }
#ifdef DEBUG
    std::cout << "Triggered kernel peeling execution" << std::endl;
#endif

    if ((loopLen + 1 + peeling + 1) <= CRF_ENTRIES) {
        ldrData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), &dummyData);    // EXIT
    }

    return 0;
}

int VectorAdditionKernel::generateVectorAdditionKernelLimC() {
    int i;
    uint loopLen;   
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    crfSegment = (CRF_ENTRIES-2)/3;
    crfSegment = (crfSegment % 2) ? crfSegment - 1 : crfSegment;    // Avoid bad alignment of memory contents
    crfSegment = std::min(crfSegment, int(2*GRF_ENTRIES));
    loops = div_ceil(numVectors*vectorDims, SIMD_WIDTH*CORES_PER_PCH) / crfSegment;
    peeling = div_ceil(numVectors*vectorDims, SIMD_WIDTH*CORES_PER_PCH) % crfSegment;

    if(loops >= (1<<11)){
        extraLoops = loops - (1<<11) + 1;
        loops = (1<<11) - 1;
    }
    else{
        extraLoops = 0;
    }

    loopLen = loops ? 3*crfSegment : 0; // MOVs, ADDs and MOVs for the 2 GRFs

    // Write instructions to the CRF for the loop
    if (loops) {
        for (i = 0; i < crfSegment/2; i++) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B
        }
        for (i = 0; i < crfSegment/2; i++) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // ADD GRF_A = GRF_A + EVEN_BANK
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_B, i, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));    // ADD GRF_B = GRF_B + ODD_BANK
        }
        for (i = 0; i < crfSegment/2; i++) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));   // MOV from GRF_A
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, i, 0, 0, 0, false));    // MOV from GRF_B
        }
        if (loops - 1) {    // Not only 1 iteration
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));  // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            cnmLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF loop instructions written" << std::endl;
#endif
    }

    // Write instructions to the CRF for the extra loop
    crfIdx = 0;
    if (extraLoops) {
        for (i = 0; i < crfSegment/2; i++) {
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B
        }
        for (i = 0; i < crfSegment/2; i++) {
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // ADD GRF_A = GRF_A + EVEN_BANK
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_B, i, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));    // ADD GRF_B = GRF_B + ODD_BANK
        }
        for (i = 0; i < crfSegment/2; i++) {
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));   // MOV from GRF_A
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, i, 0, 0, 0, false));    // MOV from GRF_B
        }
        if (extraLoops - 1) {    // Not only 1 iteration
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, extraLoops-1, 0, 0, 0, false));  // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            cnmExtraLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF extra loop instructions written" << std::endl;
#endif
    }

    // Write instructions to the CRF for the peeled part
    crfIdx = 0;
    if (peeling) {
        for (i = 0; i < peeling/2; i++) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, 0, 0, false));    // MOV to GRF_B
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, 0, 0, false));   // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // ADD GRF_A = GRF_A + EVEN_BANK
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_B, i, OPC_GRF_B, i, OPC_ODD_BANK, 0, 0, false));    // ADD GRF_B = GRF_B + ODD_BANK
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_ADD, OPC_GRF_A, i, OPC_GRF_A, i, OPC_EVEN_BANK, 0, 0, false));   // ADD GRF_A = GRF_A + EVEN_BANK
        }
        for (i = 0; i < peeling/2; i++) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));   // MOV from GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, i, 0, 0, 0, false));    // MOV from GRF_B
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_EVEN_BANK, 0, OPC_GRF_A, i, 0, 0, 0, false));   // MOV from GRF_A
        }
        if (crfIdx < CRF_ENTRIES) {
            cnmPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF peeling instructions written" << std::endl;
#endif
    }

    return 0;
}

int VectorAdditionKernel::executeVectorAdditionKernelLimC() {
    int i, j;
    uint loopLen = loops ? 3*crfSegment : 0;    // MOVs, ADDs and MOVs for the 2 GRFs
    uint64_t* execAddr = cnmElements->execAddr;
    uint vACol = vectorsA.col;
    uint vARow = vectorsA.row;
    uint vBCol = vectorsB.col;
    uint vBRow = vectorsB.row;
    uint resCol = results.col;
    uint resRow = results.row;
    uint64_t dummyData = 0;

    // Commands to write CRF instructions for the kernel
    for (auto cmd : cnmLoopSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF loop instructions" << std::endl;
#endif

    // Commands to trigger execution of the kernel loop
    for (i = 0; i < loops; i++) {
        for (j = 0; j < crfSegment/2; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
            nextColAllBanks(&vARow, &vACol);
        }
        for (j = 0; j < crfSegment/2; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
            ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_B = GRF_B + ODD_BANK
            nextColAllBanks(&vBRow, &vBCol);
        }
        for (j = 0; j < crfSegment/2; j++) {
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);   // MOV from GRF_A
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV from GRF_B
            nextColAllBanks(&resRow, &resCol);
        }
        if (loops - 1) {    // Not only 1 iteration
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // JUMP to start
        }  
    }
    if ((loops-1) > 0 && (loopLen + 2) <= CRF_ENTRIES) {    // Not only 1 iteration
        ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // EXIT
    } else if (loops && (loopLen + 1) <= CRF_ENTRIES) { // If only 1 iteration, we don't jump but we do exit
        ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // EXIT
    }
#ifdef DEBUG
    std::cout << "Triggered kernel loop execution" << std::endl;
#endif

    // Commands to write CRF instructions for the kernel
    for (auto cmd : cnmExtraLoopSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF Extra loop instructions" << std::endl;
#endif

    // Commands to trigger execution of the kernel loop
    for (i = 0; i < extraLoops; i++) {
        for (j = 0; j < crfSegment/2; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
            nextColAllBanks(&vARow, &vACol);
        }
        for (j = 0; j < crfSegment/2; j++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
            ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_B = GRF_B + ODD_BANK
            nextColAllBanks(&vBRow, &vBCol);
        }
        for (j = 0; j < crfSegment/2; j++) {
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);   // MOV from GRF_A
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV from GRF_B
            nextColAllBanks(&resRow, &resCol);
        }
        if (extraLoops - 1) {    // Not only 1 iteration
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // JUMP to start
        }  
    }
    if ((extraLoops-1) > 0 && (loopLen + 2) <= CRF_ENTRIES) {    // Not only 1 iteration
        ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // EXIT
    } else if (extraLoops && (loopLen + 1) <= CRF_ENTRIES) { // If only 1 iteration, we don't jump but we do exit
        ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // EXIT
    }
#ifdef DEBUG
    std::cout << "Triggered kernel extra loop execution" << std::endl;
#endif

    // Commands to write CRF instructions for the peeled part
    for (auto cmd : cnmPeelSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF peeling instructions" << std::endl;
#endif

            // count = 0;
    // Commands to trigger execution of the kernel peeled part
    if (peeling) {
        for (i = 0; i < peeling/2; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 1, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_B
            nextColAllBanks(&vARow, &vACol);
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 0, vARow, vACol, execAddr), &dummyData);    // MOV to GRF_A
        }
        for (i = 0; i < peeling/2; i++) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
            ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_B = GRF_B + ODD_BANK
            nextColAllBanks(&vBRow, &vBCol);    
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData);    // ADD GRF_A = GRF_A + EVEN_BANK
        }   
        for (i = 0; i < peeling/2; i++) {
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);   // MOV from GRF_A
            strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV from GRF_B
            nextColAllBanks(&resRow, &resCol);
        }
        if (peeling % 2) {  // If odd, last one is only with GRF_A
            strData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), dummyData);   // MOV from GRF_A
        }
        if ((peeling + 1) <= CRF_ENTRIES) {
            ldrData(cnmExecAddress(channel, 0, 0, 0, resRow, resCol, execAddr), &dummyData);  // EXIT
        }
#ifdef DEBUG
        std::cout << "Triggered kernel peeling execution" << std::endl;
#endif
    }

    return 0;
}

#endif  // CNM_VA_H