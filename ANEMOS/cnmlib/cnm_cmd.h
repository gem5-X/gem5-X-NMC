/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Functions to generate CnM DRAM commands
 *
 */	

#ifndef CNM_CMD_H
#define CNM_CMD_H

#include "cnm_utils.h"
#include "opcodes.h"

typedef struct CnmCmd {
    bool WR_nRD;
    uint64_t* addr;
    uint64_t data;
} CnmCmd;

CnmCmd writeCRF(uint crfIdx, uint64_t* rfAddr, OPCODE_VALUES opcode, uint dst, uint dstIdx,
                  uint src0, uint src0Idx, uint src1, uint src1Idx, uint src2, bool relu) {
    CnmCmd cmd;
    cmd.WR_nRD = true;
    cmd.data = 0;
    // Generate address to write to CRF
    cmd.addr = rfAddr + (CRF_OFFSET + (crfIdx << SHIFT_COL))/8;
    // Generate FIMDRAM instruction words
    switch (opcode) {   // TODO AAM
        case OP_NOP:
        case OP_JUMP:
        case OP_EXIT:
            cmd.data |= (opcode & ((1UL << (OPCODE_STA-OPCODE_END+1)) - 1)) << OPCODE_END;
            cmd.data |= (dstIdx & ((1UL << (IMM0_STA-IMM0_END+1)) - 1)) << IMM0_END;
            cmd.data |= (src0Idx & ((1UL << (IMM1_STA-IMM1_END+1)) - 1)) << IMM1_END;
        break;

        case OP_MOV:
        case OP_FILL:
            cmd.data |= (opcode & ((1UL << (OPCODE_STA-OPCODE_END+1)) - 1)) << OPCODE_END;
            cmd.data |= (dst & ((1UL << (DST_STA-DST_END+1)) - 1)) << DST_END;
            cmd.data |= (src0 & ((1UL << (SRC0_STA-SRC0_END+1)) - 1)) << SRC0_END;
            cmd.data |= (dstIdx & ((1UL << (DST_N_STA-DST_N_END+1)) - 1)) << DST_N_END;
            cmd.data |= (src0Idx & ((1UL << (SRC0_N_STA-SRC0_N_END+1)) - 1)) << SRC0_N_END;
            cmd.data |= (relu ? 1 : 0) << RELU_BIT;
        break;

        case OP_ADD:
        case OP_MUL:
        case OP_MAD:
        case OP_MAC:
            cmd.data |= (opcode & ((1UL << (OPCODE_STA-OPCODE_END+1)) - 1)) << OPCODE_END;
            cmd.data |= (dst & ((1UL << (DST_STA-DST_END+1)) - 1)) << DST_END;
            cmd.data |= (src0 & ((1UL << (SRC0_STA-SRC0_END+1)) - 1)) << SRC0_END;
            cmd.data |= (src1 & ((1UL << (SRC1_STA-SRC1_END+1)) - 1)) << SRC1_END;
            cmd.data |= (src2 & ((1UL << (SRC2_STA-SRC2_END+1)) - 1)) << SRC2_END;
            cmd.data |= (dstIdx & ((1UL << (DST_N_STA-DST_N_END+1)) - 1)) << DST_N_END;
            cmd.data |= (src0Idx & ((1UL << (SRC0_N_STA-SRC0_N_END+1)) - 1)) << SRC0_N_END;
            cmd.data |= (src1Idx & ((1UL << (SRC1_N_STA-SRC1_N_END+1)) - 1)) << SRC1_N_END;
        break;
        
        default:
            std::cout << "Opcode not implemented" << std::endl;
        break;
    }
    return cmd;
}

CnmCmd writeExec(bool WR_nRD, uint64_t* addr) {
    CnmCmd cmd;
    cmd.WR_nRD = WR_nRD;
    cmd.addr = addr;
    cmd.data = 0;
    return cmd;
}

CnmCmd writeSRFM(uint srfIdx, uint64_t* rfAddr, uint64_t data) {
    CnmCmd cmd;
    cmd.WR_nRD = true;
    cmd.data = data;
    cmd.addr = rfAddr + (SRFM_OFFSET + (srfIdx << SHIFT_COL))/8;
    return cmd;
}

CnmCmd writeSRFA(uint srfIdx, uint64_t* rfAddr, uint64_t data) {
    CnmCmd cmd;
    cmd.WR_nRD = true;
    cmd.data = data;
    cmd.addr = rfAddr + (SRFA_OFFSET + (srfIdx << SHIFT_COL))/8;
    return cmd;
}

#endif  // CNM_CMD_H