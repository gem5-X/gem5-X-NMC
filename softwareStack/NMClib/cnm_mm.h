/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Functions to compute the matrix multiplication CnM kernel
 * Amxn * Bnxq = Cmxq 
 *
 */

#ifndef CNM_MM_H
#define CNM_MM_H

#include "cnm_utils.h"
#include "cnm_kernel.h"

class MatrixMultiplicationKernel : public Kernel {
    protected:
        int m, n, q;
        uint64_t* matrixAdata;
        DataIdx matrixB;
        DataIdx matrixRes;

        std::vector<CnmCmd> crfExtLoopSeq, crfExtPeelSeq;
        std::vector<CnmCmd> srfExtLoopSeq, srfExtPeelSeq;

        int ext_loops, loops, ext_peeling;
        int crfSegment;

        int allocateMatrixMultiplicationKernel();
        int generateMatrixMultiplicationKernelLimR();
        int generateMatrixMultiplicationKernelLimC();
        int executeMatrixMultiplicationKernelLimR();
        int executeMatrixMultiplicationKernelLimC();

    public:
        MatrixMultiplicationKernel (CnmElements* _cnmElements, uint _channel, uint _m, uint _n, uint _q);
        ~MatrixMultiplicationKernel();

        int generateSequence();
        int executeSequence();
        void storeKernel (uint64_t* mAdata, uint64_t* mBdata);
        void storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData);
        void loadResults (uint64_t* mResData);
};

MatrixMultiplicationKernel::MatrixMultiplicationKernel (CnmElements* _cnmElements, uint _channel, uint _m, uint _n, uint _q) : 
    Kernel(_cnmElements, KernelType::MATRIX_MULT, _channel) {
        m = _m;
        n = _n;
        q = _q;
        limC = CRF_ENTRIES < (SRF_M_ENTRIES + 4);
        if (allocateMatrixMultiplicationKernel()) {
            std::cout << "Error allocating memory for the matrix multiplication kernel" << std::endl;
            exit(1);
        }
        matrixAdata = NULL;
        matrixB.row = rowStart;
        matrixB.col = 0;
        matrixRes.row = rowStart + (div_ceil(n, 2)*div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG)) / NUM_COL;
        matrixRes.col = (div_ceil(n, 2)*div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG)) % NUM_COL;
        addrStart = cnmExecAddress(channel, 0, 0, 0, rowStart, 0, cnmElements->execAddr);
#ifdef DEBUG
        std::cout << "Kernel is " << (limC ? "C-limited" : "R-limited") << std::endl;
#endif
}

MatrixMultiplicationKernel::~MatrixMultiplicationKernel() {
    // Find itself in cnmElements and delete the reference
    for (auto it = cnmElements->kernelList[channel].begin(); it != cnmElements->kernelList[channel].end(); it++) {
        if (*it == this) {
            cnmElements->kernelList[channel].erase(it);
            break;
        }
    }
    // Clear the sequence
    cnmSequence.clear();
    crfExtLoopSeq.clear();
    crfExtPeelSeq.clear();
    srfExtLoopSeq.clear();
    srfExtPeelSeq.clear();
}

int MatrixMultiplicationKernel::generateSequence() {
    if (limC) {
        return generateMatrixMultiplicationKernelLimC();
    } else {
        return generateMatrixMultiplicationKernelLimR();
    }
}

int MatrixMultiplicationKernel::executeSequence() {
    uint64_t dummyData = 0;
    int error;

    switchCnmMode(channel, &dummyData, cnmElements->rfAddr);    
    if (limC) {
        error = executeMatrixMultiplicationKernelLimC();
    } else {
        error = executeMatrixMultiplicationKernelLimR();
    }
    switchCnmMode(channel, &dummyData, cnmElements->rfAddr); 
    return error;
}

void MatrixMultiplicationKernel::storeKernel (uint64_t* mAdata, uint64_t* mBdata) {
    // We assume that the matrices are stored in row-major order
    uint totalCol = div_ceil(q, SIMD_WIDTH);
    uint colBIdx = matrixB.col;
    uint rowBIdx = matrixB.row;
    uint bankBIdx = 0;
    uint bgBIdx = 0;
    uint bankBParity = 0;

    // Store the matrix A
    matrixAdata = mAdata;

    // Store the matrix B, row by row, in DRAM-column chunks, jumper over channels and offset bits
    for (int i = 0; i < n; i++) {
        for (uint j = 0; j < totalCol; j++) {
#ifndef CHECKER
            memcpy(cnmExecAddress(channel, 0, bgBIdx, bankBIdx + bankBParity, rowBIdx, colBIdx, cnmElements->execAddr), mBdata + (i*totalCol + j)*GRF_64B, GRF_64B*sizeof(uint64_t));
#else
            memcpy(addrStart + (i*totalCol + j)*GRF_64B, mBdata + (i*totalCol + j)*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Storing index " << std::showbase <<  std::dec << (i*totalCol + j) << " in B: ";
            std::cout << std::hex << mBdata[(i*totalCol + j)*GRF_64B];
            std::cout << std::dec << " bg " << bgBIdx << " bank " << bankBIdx + bankBParity << " row " << rowBIdx << " col " << colBIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgBIdx, &bankBIdx, &rowBIdx, &colBIdx);
        }
        // Start with aligned bg and bank, changing parity and column as needed (store rows in alternating bank parities)
        changeParityJumpBackCol(&bgBIdx, &bankBIdx, &rowBIdx, &colBIdx, &bankBParity, div_ceil(totalCol, (NUM_BG*NUM_BANK/2)));
    }

    // Initialice the results to 0
    uint colIdx = matrixRes.col;
    uint rowIdx = matrixRes.row;
    uint bankIdx = 1;
    uint bgIdx = 0;
    uint64_t zero[GRF_64B] = {0};

    for (int i = 0; i < m; i++) {
        for (uint j = 0; j < totalCol; j++) {
#ifndef CHECKER
            memcpy(cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr), zero, GRF_64B*sizeof(uint64_t));
#else
            memcpy(addrStart + n*totalCol*GRF_64B + (i*totalCol+j)*GRF_64B, zero, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Pre-storing 0 in the result " << std::showbase <<  std::dec << (i*totalCol + j) << ": ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr) << std::dec;
            std::cout << std::dec << " bg " << bgIdx << " bank " << bankIdx << " row " << rowIdx << " col " << colIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgIdx, &bankIdx, &rowIdx, &colIdx); // All results come from GRF_B
        }
        if (bankIdx != 1 || bgIdx != 0) {  
            bgIdx = 0;
            bankIdx = 1;
            nextCol(&rowIdx, &colIdx);
        }
    }

#ifdef DEBUG
    std::cout << "Kernel data stored" << std::endl;
#endif
}

void MatrixMultiplicationKernel::storeKernel (uint64_t* inputData, uint64_t* weightsData, uint64_t* biasData) {
    std::cout << "Error, storing convolution kernels needs two inputs: M1 and M2";
    exit(1);
}

void MatrixMultiplicationKernel::loadResults (uint64_t* mResData) {
    uint totalCol = div_ceil(q, SIMD_WIDTH);
    uint colIdx = matrixRes.col;
    uint rowIdx = matrixRes.row;
    uint bankIdx = 1;
    uint bgIdx = 0;

    for (int i = 0; i < m; i++) {
        for (uint j = 0; j < totalCol; j++) {
#ifndef CHECKER
            memcpy(mResData + (i*totalCol+j)*GRF_64B, cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr), GRF_64B*sizeof(uint64_t));
#else
            memcpy(mResData + (i*totalCol+j)*GRF_64B, addrStart + n*totalCol*GRF_64B + (i*totalCol+j)*GRF_64B, GRF_64B*sizeof(uint64_t));
#endif
#ifdef DEBUG
            std::cout << "Retrieving index " << std::dec <<  (i*totalCol + j) << " in ";
            std::cout << "address " << std::hex << cnmExecAddress(channel, 0, bgIdx, bankIdx, rowIdx, colIdx, cnmElements->execAddr) << std::dec;
            std::cout << " contents " << std::showbase << std::hex << mResData[(i*totalCol+j)] << std::dec;
            std::cout << std::dec << " bg " << bgIdx << " bank " << bankIdx << " row " << rowIdx << " col " << colIdx << std::endl;
#endif
            nextColChunkBgBaKeepBankParity(&bgIdx, &bankIdx, &rowIdx, &colIdx); // All results come from GRF_B
        }
        if (bankIdx != 1 || bgIdx != 0) {  
            bgIdx = 0;
            bankIdx = 1;
            nextCol(&rowIdx, &colIdx);
        }
    }
#ifdef DEBUG
    std::cout << "Kernel results retrieved" << std::endl;
#endif
}

int MatrixMultiplicationKernel::allocateMatrixMultiplicationKernel() {
    uint rows = div_ceil((div_ceil(n, 2)+(div_ceil(m, 2)))*div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG), NUM_COL);
    return allocateKernel(rows);
}

int MatrixMultiplicationKernel::generateMatrixMultiplicationKernelLimR() {
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    ext_loops = n / SRF_M_ENTRIES;
    loops = div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    ext_peeling = n % SRF_M_ENTRIES;

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! R-lim MM. Loops = " << loops << std::endl;
    }

    if (ext_loops) {
        // Write instructions to the CRF for the external loop
        loopLen = SRF_M_ENTRIES+2;
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial product))
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

    // Write instructions to the CRF for the external peeling
    if (ext_loops && ext_peeling) {
        crfIdx = ext_peeling + 1;   // We only have to overwrite the last instructions
        loopLen = ext_peeling + 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    } else if (ext_peeling) {
        crfIdx = 0;
        loopLen = ext_peeling + 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial product))
        for (i = 0; i < ext_peeling/2; i++) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
        }
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    }

    // Write SRF contents of the external peeling
    if (ext_peeling) {
        for (i = 0; i < ext_peeling; i++) {   
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    }

    return 0;
}

int MatrixMultiplicationKernel::executeMatrixMultiplicationKernelLimR() {
    int i, j, k, l;
    uint loopLen = SRF_M_ENTRIES+2;
    uint64_t* execAddr = cnmElements->execAddr;
    uint vBCol = matrixB.col;
    uint vBRow = matrixB.row;
    uint matrixBRowLen = div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    uint resCol = matrixRes.col;
    uint resRow = matrixRes.row;
    uint64_t dummyData = 0; // Dummy data to do ldrData and strData that trigger the execution

    // Commands to write CRF instructions for the external loop
    for (auto cmd : crfExtLoopSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF external loop instructions" << std::endl;
#endif

    // Commands to write SRF contents and trigger execution of the external loop
    for (i = 0; i < m; i++) {
        for (j = 0; j < ext_loops; j++) {
            // Write SRF contents
            for (k = 0; k < SRF_M_ENTRIES; k++) {
                uint matIdx = i*(div_ceil(n,SIMD_WIDTH)*GRF_64B) + (j*SRF_M_ENTRIES + k)/WORDS_PER_64B;
                uint64_t element = matrixAdata[matIdx] >> (WORD_BITS * ((j*SRF_M_ENTRIES + k) % WORDS_PER_64B));
                strData(srfExtLoopSeq[k].addr, element);
            }
            // Trigger execution of the external loop
            for (k = 0; k < loops; k++) {
                vBRow = matrixB.row;
                vBCol = matrixB.col;
                jumpColAllBanks(&vBRow, &vBCol, j*matrixBRowLen*SRF_M_ENTRIES/2 + k);   // SRF_M_ENTRIES/2 as we account for even and odd banks

                resRow = matrixRes.row;
                resCol = matrixRes.col;
                jumpColAllBanks(&resRow, &resCol, i*matrixBRowLen + k);

                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // MOV to GRF_B (get current partial product)
                for (l = 0; l < SRF_M_ENTRIES/2; l++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += ODD_BANK * SRF_M
                    jumpColAllBanks(&vBRow, &vBCol, matrixBRowLen); 
                }
                strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) { // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            } else if (loops && loopLen < CRF_ENTRIES-1) {  // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            }
        }
#ifdef DEBUG
        std::cout << "Written SRF and triggered external loop of row " << std::dec << i << std::endl;
#endif
    }

    // Commands to write CRF instructions for the external peeling
    for (auto cmd : crfExtPeelSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

    // Commands to write SRF contents and trigger execution of the external peeling
    if (ext_peeling) {
        loopLen = ext_peeling + 2;
        for (i = 0; i < m; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint matIdx = i*(div_ceil(n,SIMD_WIDTH)*GRF_64B) + (ext_loops*SRF_M_ENTRIES + j)/WORDS_PER_64B;
                uint64_t element = matrixAdata[matIdx] >> (WORD_BITS * ((ext_loops*SRF_M_ENTRIES + j) % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                vBRow = matrixB.row;
                vBCol = matrixB.col;
                jumpColAllBanks(&vBRow, &vBCol, ext_loops*matrixBRowLen*SRF_M_ENTRIES/2 + j);   // SRF_M_ENTRIES/2 as we account for even and odd banks

                resRow = matrixRes.row;
                resCol = matrixRes.col;
                jumpColAllBanks(&resRow, &resCol, i*matrixBRowLen + j);

                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // MOV to GRF_B (get current partial product)
                for (k = 0; k < ext_peeling/2; k++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += ODD_BANK * SRF_M
                    jumpColAllBanks(&vBRow, &vBCol, matrixBRowLen); 
                }
                if (ext_peeling % 2) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                }
                strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) { // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            } else if (loops && loopLen < CRF_ENTRIES-1) {  // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            }
#ifdef DEBUG
            std::cout << "Written SRF and triggered external peeling of row " << std::dec << i << std::endl;
#endif
        }
    }

    return 0;
}

int MatrixMultiplicationKernel::generateMatrixMultiplicationKernelLimC() {
    int i;
    uint loopLen;
    uint crfIdx = 0;
    uint64_t* rfAddr = cnmElements->rfAddr;

    // Initialize kernel configuration
    crfSegment = ((CRF_ENTRIES - 4) / 2) * 2;   // Round down to even number
    crfSegment = std::min(crfSegment, SRF_M_ENTRIES);
    ext_loops = n / crfSegment;
    loops = div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    ext_peeling = n % crfSegment;

    if(loops >= (1<<11)){
        std::cout << "Warning: Number of loops exceeds IMM1 number of bits! C-lim MM. Loops = " << loops << std::endl;
    }

    if (ext_loops) {
        // Write instructions to the CRF for the external loop
        loopLen = crfSegment+2;
        crfExtLoopSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial product))
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

    // Write instructions to the CRF for the external peeling
    if (ext_loops && ext_peeling) {
        crfIdx = ext_peeling + 1;   // We only have to overwrite the last instructions
        loopLen = ext_peeling + 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false));    // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));  // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));  // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    } else if (ext_peeling) {
        crfIdx = 0;
        loopLen = ext_peeling + 2;
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_GRF_B, 0, OPC_ODD_BANK, 0, 0, 0, 0, false)); // MOV to GRF_B (get current partial product))
        for (i = 0; i < ext_peeling/2; i++) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_ODD_BANK, 0, OPC_SRF_M, 2*i+1, 0, false)); // MAC GRF_B += ODD_BANK * SRF_M
        }
        if (ext_peeling % 2) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MAC, OPC_GRF_B, 0, OPC_EVEN_BANK, 0, OPC_SRF_M, 2*i, 0, false));  // MAC GRF_B += EVEN_BANK * SRF_M
        }
        crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_MOV, OPC_ODD_BANK, 0, OPC_GRF_B, 0, 0, 0, 0, false)); // MOV to ODD_BANK
        if (loops - 1 > 0) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_JUMP, 0, loopLen, 0, loops-1, 0, 0, 0, false));   // JUMP to start
        }
        if (crfIdx < CRF_ENTRIES) {
            crfExtPeelSeq.push_back(writeCRF(crfIdx++, rfAddr, OP_EXIT, 0, 0, 0, 0, 0, 0, 0, false));   // EXIT
        }
#ifdef DEBUG
        std::cout << "CRF external peeling instructions written" << std::endl;
#endif
    }

    // Write SRF contents of the external peeling
    if (ext_peeling) {
        for (i = 0; i < ext_peeling; i++) {
            srfExtPeelSeq.push_back(writeSRFM(i, rfAddr, 0));
        }
#ifdef DEBUG
        std::cout << "SRF for external peeling written" << std::endl;
#endif
    }

    return 0;
}

int MatrixMultiplicationKernel::executeMatrixMultiplicationKernelLimC() {
    int i, j, k, l;
    uint loopLen = crfSegment+2;
    uint64_t* execAddr = cnmElements->execAddr;
    uint vBCol = matrixB.col;
    uint vBRow = matrixB.row;
    uint matrixBRowLen = div_ceil(q, SIMD_WIDTH*(NUM_BANK/2)*NUM_BG);
    uint resCol = matrixRes.col;
    uint resRow = matrixRes.row;
    uint64_t dummyData = 0; // Dummy data to do ldrData and strData that trigger the execution

    // Commands to write CRF instructions for the external loop
    for (auto cmd : crfExtLoopSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
        std::cout << "Writing CRF external loop instructions" << std::endl;
#endif

    // Commands to write SRF contents and trigger execution of the external loop
    for (i = 0; i < m; i++) {
        for (j = 0; j < ext_loops; j++) {
            // Write SRF contents
            for (k = 0; k < crfSegment; k++) {
                uint matIdx = i*(div_ceil(n,SIMD_WIDTH)*GRF_64B) + (j*crfSegment + k)/WORDS_PER_64B;
                uint64_t element = matrixAdata[matIdx] >> (WORD_BITS * ((j*crfSegment + k) % WORDS_PER_64B));
                strData(srfExtLoopSeq[k].addr, element);
            }
            // Trigger execution of the external loop
            for (k = 0; k < loops; k++) {
                vBRow = matrixB.row;
                vBCol = matrixB.col;
                jumpColAllBanks(&vBRow, &vBCol, j*matrixBRowLen*crfSegment/2 + k);  // crfSegment/2 as we account for even and odd banks

                resRow = matrixRes.row;
                resCol = matrixRes.col;
                jumpColAllBanks(&resRow, &resCol, i*matrixBRowLen + k);

                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // MOV to GRF_B (get current partial product)
                for (l = 0; l < crfSegment/2; l++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += ODD_BANK * SRF_M
                    jumpColAllBanks(&vBRow, &vBCol, matrixBRowLen); 
                }
                strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) { // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            } else if (loops && loopLen < CRF_ENTRIES-1) {  // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            }
        }
#ifdef DEBUG
        std::cout << "Written SRF and triggered external loop of row " << std::dec << i << std::endl;
#endif
    }

    // Commands to write CRF instructions for the external peeling
    for (auto cmd : crfExtPeelSeq) {
        strData(cmd.addr, cmd.data);
    }
#ifdef DEBUG
    std::cout << "Writing CRF external peeling instructions" << std::endl;
#endif

    // Commands to write SRF contents and trigger execution of the external peeling
    if (ext_peeling) {
        loopLen = ext_peeling + 2;
        for (i = 0; i < m; i++) {
            // Write SRF contents
            for (j = 0; j < ext_peeling; j++) {
                uint matIdx = i*(div_ceil(n,SIMD_WIDTH)*GRF_64B) + (ext_loops*crfSegment + j)/WORDS_PER_64B;
                uint64_t element = matrixAdata[matIdx] >> (WORD_BITS * ((ext_loops*crfSegment + j) % WORDS_PER_64B));
                strData(srfExtPeelSeq[j].addr, element);
            }
            // Trigger execution of the external peeling
            for (j = 0; j < loops; j++) {
                vBRow = matrixB.row;
                vBCol = matrixB.col;
                jumpColAllBanks(&vBRow, &vBCol, ext_loops*matrixBRowLen*crfSegment/2 + j);  // crfSegment/2 as we account for even and odd banks

                resRow = matrixRes.row;
                resCol = matrixRes.col;
                jumpColAllBanks(&resRow, &resCol, i*matrixBRowLen + j);

                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // MOV to GRF_B (get current partial product)
                for (k = 0; k < ext_peeling/2; k++) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                    ldrData(cnmExecAddress(channel, 0, 0, 1, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += ODD_BANK * SRF_M
                    jumpColAllBanks(&vBRow, &vBCol, matrixBRowLen); 
                }
                if (ext_peeling % 2) {
                    ldrData(cnmExecAddress(channel, 0, 0, 0, vBRow, vBCol, execAddr), &dummyData); // MAC GRF_B += EVEN_BANK * SRF_M
                }
                strData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), dummyData);   // MOV to ODD_BANK
                if (loops - 1) {    // Not only 1 iteration
                    ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // JUMP to start
                }
            }
            if ((loops-1) > 0 && loopLen + 1 < CRF_ENTRIES-1) { // Not only 1 iteration
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            } else if (loops && loopLen < CRF_ENTRIES-1) {  // If only 1 iteration, we don't jump but we do exit
                ldrData(cnmExecAddress(channel, 0, 0, 1, resRow, resCol, execAddr), &dummyData);   // EXIT
            }
#ifdef DEBUG
            std::cout << "Written SRF and triggered external peeling of row " << std::dec << i << std::endl;
#endif
        }
    }

    return 0;
}

#endif // CNM_MM_H