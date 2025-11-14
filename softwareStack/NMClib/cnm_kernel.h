/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Class and functions to configure and employ CnM kernels
 *
 */	

#ifndef CNM_KERNEL_H
#define CNM_KERNEL_H

#include "cnm_utils.h"
#include "cnm_cmd.h"

enum class KernelType {
    VECTOR_ADDITION,
    DOT_PRODUCT,
    MATRIX_MULT,
    CONVOLUTION
};

class Kernel;

typedef struct CnmElements {
    uint64_t* execAddr;
    uint64_t* rfAddr;
    uint numChannels;
    std::vector<std::vector<Kernel*> > kernelList;

    CnmElements(uint _numChannels) : execAddr(NULL), rfAddr(NULL), numChannels(_numChannels) {
        kernelList.resize(_numChannels);
    }
} CnmElements;

typedef struct DataIdx {
    uint row;
    uint col;
} DataIdx;

// Struct / class to store the kernel configuration
class Kernel  {
    protected:
        CnmElements* cnmElements;
        KernelType type;
        bool limC;      // True: limited by number of control regs, False: limited by number of data regs
        uint channel;
        uint rowStart;  // First DRAM row occupied by the kernel
        uint rowEnd;    // Last DRAM row occupied by the kernel
        uint64_t* addrStart;    // Address equivalent to rowStart
        std::vector<CnmCmd> cnmSequence;

        int allocateKernel(uint rows) {
            // Create default start and end in case the kernel list is empty
            uint rowStartTmp = 0;
            uint rowEndTmp = rows-1;
            // Run through the kernel list to find the free row
            if (!cnmElements->kernelList[channel].empty()) {
                for (auto kernel : cnmElements->kernelList[channel]) {
                    if (kernel->rowStart <= rowEndTmp || kernel->rowEnd >= rowStartTmp) {
                        rowStartTmp = kernel->rowEnd + 1;
                        rowEndTmp = rowStart + rows - 1;
                    }
                }
            }
            if (rowEndTmp >= NUM_ROW) {
                std::cout << "Error, not enough memory for the kernel" << std::endl;
                return -1;
            } else {
                rowStart = rowStartTmp;
                rowEnd = rowEndTmp;
                return 0;
            }
        }

    public:

        Kernel(CnmElements* _cnmElements, KernelType _type, uint _channel) : 
            cnmElements(_cnmElements), type(_type), channel(_channel) {
                rowStart = 0;
                rowEnd = 0;
                addrStart = NULL;
        }

        virtual ~Kernel() {};

        uint getRowStart() {
            return rowStart;
        }

        virtual int generateSequence() = 0;
        virtual int executeSequence() {
            for (auto cmd : cnmSequence) {
#ifdef DEBUG
#ifndef CHECKER
                std::cout << "Executing command " << std::hex << cmd.addr << " ";
                std::cout << (cmd.WR_nRD ? "WR" : "RD") << " " << cmd.data << std::endl;
#endif
#endif
                if (cmd.WR_nRD) {
                    strData(cmd.addr, cmd.data);
                } else {
                    ldrData(cmd.addr, &cmd.data);
                }
            }
#ifdef DEBUG
            std::cout << "Kernel sequence executed" << std::endl;
#endif
            return 0;
        }

        virtual void storeKernel(uint64_t* vAdata, uint64_t* vBdata) = 0;
        virtual void storeKernel(uint64_t* input, uint64_t* weights, uint64_t* bias) = 0;
        virtual void loadResults(uint64_t* resData) = 0;
};

uint64_t* getLastAddr(std::vector<CnmCmd> cnmSequence) {
    uint64_t* lastAddr = NULL;
    lastAddr = cnmSequence.back().addr;
    return lastAddr;
}

#endif  // CNM_KERNEL_H