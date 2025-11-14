#include "systemc.h"
#include <bitset>

#include "../cnm_base.h"

SC_MODULE(cnm_monitor) {

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 RD[NUM_CHANNEL];                         // DRAM read command
    sc_in<bool>                 WR[NUM_CHANNEL];                         // DRAM write command
    sc_in<bool>                 ACT[NUM_CHANNEL];                        // DRAM activate command
//    sc_in<bool>                   RSTB[NUM_CHANNEL];                       //
    sc_in<bool>                 AB_mode[NUM_CHANNEL];                    // Signals if the All-Banks mode is enabled
    sc_in<bool>                 pim_mode[NUM_CHANNEL];                   // Signals if the PIM mode is enabled
    sc_in<sc_uint<BANK_BITS> >  bank_addr[NUM_CHANNEL];                  // Address of the bank
    sc_in<sc_uint<ROW_BITS> >   row_addr[NUM_CHANNEL];                   // Address of the bank row
    sc_in<sc_uint<COL_BITS> >   col_addr[NUM_CHANNEL];                   // Address of the bank column
    sc_in<sc_uint<DQ_BITS> >    DQ[NUM_CHANNEL];                         // Data input from DRAM controller (output makes no sense)
    sc_inout_rv<GRF_WIDTH>      even_buses[NUM_CHANNEL][CORES_PER_PCH];  // Direct data in/out to the even banks
    sc_inout_rv<GRF_WIDTH>      odd_buses[NUM_CHANNEL][CORES_PER_PCH];   // Direct data in/out to the odd banks

    // Internal events

    // Internal variables and signals for checking

    SC_CTOR(cnm_monitor) {
        SC_THREAD(monitor_thread);
        sensitive << clk.pos() << rst.neg();
    }

    void monitor_thread(); // Outputs the behavior and automatically checks the functionality
};
