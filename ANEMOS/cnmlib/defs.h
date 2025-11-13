 /*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 * Riselda Kodra
 *
 * Defines for configuring the design.
 *
 */

#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

#define MIXED_SIM   0   // 0 if SystemC-only simulation, 1 if mixed SystemC + RTL
#define GEM5        1
#define OUTPUT_LOG  1
// #define DEBUG       0

#define RESOLUTION SC_PS

// 0: half
// 1: float
// 2: double
// 3: bfloat (not implemented)
// 4: int8
// 5: int16
// 6: int32
// 7: int64
#define DATA_TYPE   0
// Address mapping constants
// 0: HBM2_AB
// 1: DDR4_AB
// 2: GDDR5_AB
// 3: LPDDR4_AB

#define DRAM 1

#if (DRAM == 0)
    #define CLK_PERIOD 3333
    #define CHANNEL_BITS    4
    #define RANK_BITS       0
    #define BG_BITS         2
    #define BANK_BITS       2
    #define ROW_BITS        14
    #define COL_BITS        5
    #define GLOBAL_OFFSET   6
#elif (DRAM == 1)
    #define CLK_PERIOD 2500
    #define CHANNEL_BITS    0
    #define RANK_BITS       0
    #define BG_BITS         2 //changed header file
    #define BANK_BITS       2
    #define ROW_BITS        15
    #define COL_BITS        7 // 7+3 Rafa's script
    #define GLOBAL_OFFSET   6
#elif (DRAM == 2)
    #define CLK_PERIOD 1000
    #define CHANNEL_BITS    0
    #define RANK_BITS       0   //there is 1 rank, Rafa's scripts no sed for this
    #define BG_BITS         2
    #define BANK_BITS       2
    #define ROW_BITS        14
    #define COL_BITS        7 // it is defined as 7+3 in .h file
    #define GLOBAL_OFFSET   6
#elif (DRAM == 3)
    #define CLK_PERIOD 5000
    #define CHANNEL_BITS    1
    #define RANK_BITS       0
    #define BG_BITS         0
    #define BANK_BITS       3
    #define ROW_BITS        15
    #define COL_BITS        6 //6+4
    #define GLOBAL_OFFSET   6
#endif

// Sizing constants
#define NUM_CHANNEL     1
#if (DRAM == 0)
    #define CORES_PER_PCH   8
	#define GRF_WIDTH		256
#elif (DRAM == 1)
    #define CORES_PER_PCH   8
	#define GRF_WIDTH		64
#elif (DRAM == 2)
    #define CORES_PER_PCH   8
	#define GRF_WIDTH		256
#elif (DRAM == 3)
    #define CORES_PER_PCH   4
	#define GRF_WIDTH		256
#endif
#define CRF_ENTRIES     32
#define SRF_A_ENTRIES   8
#define SRF_M_ENTRIES   8
#define GRF_ENTRIES     8
#define ADD_STAGES      1
#define MULT_STAGES     1
#define RF_SEL_BITS     ROW_BITS-1
#define RF_ADDR_BITS    COL_BITS
#define AAM_ADDR_BITS   3
#define INSTR_BITS      32
#define WORD_BITS       16
// #define GRF_WIDTH       (WORD_BITS*SIMD_WIDTH)
#define DQ_BITS         64
#define DQ_CLK          (GRF_WIDTH/DQ_BITS)
#define INSTR_CLK       (INSTR_BITS/DQ_BITS)
#define SIMD_WIDTH		(GRF_WIDTH/WORD_BITS)
#define DWORDS_PER_COL	(GRF_WIDTH/64) //In cnmlib there is GRF_64B which basically the same thing, kept it unchaned to avoid bugs for the moment

// If not enough column bits to address CRF, using also bank bits
#define CRF_BANK_ADDR   ((1 << COL_BITS) < CRF_ENTRIES)

// Define to use or not assert library
#ifndef DEBUG 
#define NDEBUG
#endif

#define SEED    0

//Uncomment if you want to debug with prints
//#define DBGPRINTS

#endif /* SRC_DEFS_H_ */