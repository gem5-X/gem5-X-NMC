/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Description of a Compute-near-Memory device that contains several channels, each with several IMC cores
 *
 */

#ifndef SRC_CNM_DEVICE_H_
#define SRC_CNM_DEVICE_H_

#include "imc_pch.h"

class cnm_device: public sc_module {
    public:

    sc_in_clk                       clk;
    sc_in<bool>                     rst;
    sc_in<bool>                     RD[NUM_CHANNEL];                           // DRAM read command
    sc_in<bool>                     WR[NUM_CHANNEL];                           // DRAM write command
    sc_in<bool>                     ACT[NUM_CHANNEL];                          // DRAM activate command
//    sc_in<bool>                       RSTB[NUM_CHANNEL];                         //
    sc_in<bool>                     AB_mode[NUM_CHANNEL];                      // Signals if the All-Banks mode is enabled
    sc_in<bool>                     pim_mode[NUM_CHANNEL];                     // Signals if the PIM mode is enabled
//    sc_in<sc_uint<CHANNEL_BITS> >   channel_addr;                               // Address of the channel
    sc_in<sc_uint<BANK_BITS> >      bank_addr[NUM_CHANNEL];                    // Address of the bank
    sc_in<sc_uint<ROW_BITS> >       row_addr[NUM_CHANNEL];                     // Address of the bank row
    sc_in<sc_uint<COL_BITS> >       col_addr[NUM_CHANNEL];                     // Address of the bank column
    sc_in<sc_uint<DQ_BITS> >        DQ[NUM_CHANNEL];                           // Data input from DRAM controller (output makes no sense)
    sc_inout_rv<GRF_WIDTH>          even_buses[NUM_CHANNEL][CORES_PER_PCH];    // Direct data in/out to the even banks
    sc_inout_rv<GRF_WIDTH>          odd_buses[NUM_CHANNEL][CORES_PER_PCH];     // Direct data in/out to the odd banks

    // ** INTERNAL SIGNALS AND VARIABLES **

    // Auxiliar signals

    // Internal modules
    imc_pch *imc_pchs[NUM_CHANNEL]; // Vector of IMC cores

    SC_CTOR(cnm_device) {

        uint i, j;

        for (i = 0; i < NUM_CHANNEL; i++) {
            imc_pchs[i] = new imc_pch(sc_gen_unique_name("imc_pch"));
            imc_pchs[i]->clk(clk);
            imc_pchs[i]->rst(rst);
            imc_pchs[i]->RD(RD[i]);
            imc_pchs[i]->WR(WR[i]);
            imc_pchs[i]->ACT(ACT[i]);
//          imc_pchs[i]->RSTB(RSTB[i]);
            imc_pchs[i]->AB_mode(AB_mode[i]);
            imc_pchs[i]->pim_mode(pim_mode[i]);
            imc_pchs[i]->bank_addr(bank_addr[i]);
            imc_pchs[i]->row_addr(row_addr[i]);
            imc_pchs[i]->col_addr(col_addr[i]);
            imc_pchs[i]->DQ(DQ[i]);
            for (j = 0; j < CORES_PER_PCH; j++) {
                imc_pchs[i]->even_buses[j](even_buses[i][j]);
                imc_pchs[i]->odd_buses[j](odd_buses[i][j]);
            }
        }

    }
};

#endif /* SRC_CNM_DEVICE_H_ */
