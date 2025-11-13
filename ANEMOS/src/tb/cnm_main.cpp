#include "cnm_main.h"

int sc_main(int argc, char *argv[]) {

    sc_clock                        clk("clk", CLK_PERIOD, RESOLUTION);
    sc_signal<bool>                 rst;
    sc_signal<bool>                 RD[NUM_CHANNEL];                         // DRAM read command
    sc_signal<bool>                 WR[NUM_CHANNEL];                         // DRAM write command
    sc_signal<bool>                 ACT[NUM_CHANNEL];                        // DRAM activate command
//    sc_signal<bool>                   RSTB[NUM_CHANNEL];                       //
    sc_signal<bool>                 AB_mode[NUM_CHANNEL];                    // Signals if the All-Banks mode is enabled
    sc_signal<bool>                 pim_mode[NUM_CHANNEL];                   // Signals if the PIM mode is enabled
    sc_signal<sc_uint<BANK_BITS> >  bank_addr[NUM_CHANNEL];                  // Address of the bank
    sc_signal<sc_uint<ROW_BITS> >   row_addr[NUM_CHANNEL];                   // Address of the bank row
    sc_signal<sc_uint<COL_BITS> >   col_addr[NUM_CHANNEL];                   // Address of the bank column
    sc_signal<sc_uint<DQ_BITS> >    DQ[NUM_CHANNEL];                         // Data input from DRAM controller (output makes no sense
    sc_signal_rv<GRF_WIDTH>         even_buses[NUM_CHANNEL][CORES_PER_PCH];  // Direct data in/out to the even bank
    sc_signal_rv<GRF_WIDTH>         odd_buses[NUM_CHANNEL][CORES_PER_PCH];   // Direct data in/out to the odd bank

    uint i, j;

    cnm_device dut("CnMDeviceUnderTest");
    dut.clk(clk);
    dut.rst(rst);
    for (i = 0; i < NUM_CHANNEL; i++) {
        dut.RD[i](RD[i]);
        dut.WR[i](WR[i]);
        dut.ACT[i](ACT[i]);
    //    dut.RSTB[i](RSTB[i]);
        dut.AB_mode[i](AB_mode[i]);
        dut.pim_mode[i](pim_mode[i]);
        dut.bank_addr[i](bank_addr[i]);
        dut.row_addr[i](row_addr[i]);
        dut.col_addr[i](col_addr[i]);
        dut.DQ[i](DQ[i]);
        for (j = 0; j < CORES_PER_PCH; j++) {
            dut.even_buses[i][j](even_buses[i][j]);
            dut.odd_buses[i][j](odd_buses[i][j]);
        }
    }

#if (GEM5)
    cnm_driver driver("Driver", std::string(argv[1]), std::string(argv[2]));
#else
    cnm_driver driver("Driver", std::string(argv[1]));
#endif
    driver.rst(rst);
    for (i = 0; i < NUM_CHANNEL; i++) {
        driver.RD[i](RD[i]);
        driver.WR[i](WR[i]);
        driver.ACT[i](ACT[i]);
    //    driver.RSTB[i](RSTB);
        driver.AB_mode[i](AB_mode[i]);
        driver.pim_mode[i](pim_mode[i]);
        driver.bank_addr[i](bank_addr[i]);
        driver.row_addr[i](row_addr[i]);
        driver.col_addr[i](col_addr[i]);
        driver.DQ[i](DQ[i]);
        for (j = 0; j < CORES_PER_PCH; j++) {
            driver.even_buses[i][j](even_buses[i][j]);
            driver.odd_buses[i][j](odd_buses[i][j]);
        }
    }

    cnm_monitor monitor("Monitor");
    monitor.clk(clk);
    monitor.rst(rst);
    for (i = 0; i < NUM_CHANNEL; i++) {
        monitor.RD[i](RD[i]);
        monitor.WR[i](WR[i]);
        monitor.ACT[i](ACT[i]);
    //    monitor.RSTB[i](RSTB[i]);
        monitor.AB_mode[i](AB_mode[i]);
        monitor.pim_mode[i](pim_mode[i]);
        monitor.bank_addr[i](bank_addr[i]);
        monitor.row_addr[i](row_addr[i]);
        monitor.col_addr[i](col_addr[i]);
        monitor.DQ[i](DQ[i]);
        for (j = 0; j < CORES_PER_PCH; j++) {
            monitor.even_buses[i][j](even_buses[i][j]);
            monitor.odd_buses[i][j](odd_buses[i][j]);
        }
    }

    sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_,
            SC_DO_NOTHING);
    sc_report_handler::set_actions (SC_WARNING, SC_DO_NOTHING);

    sc_trace_file *tracefile;
#if !(GEM5)
        tracefile = sc_create_vcd_trace_file("loops2049");
#endif

sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
    sc_trace(tracefile, RD[0], "RD");
    sc_trace(tracefile, WR[0], "WR");
    sc_trace(tracefile, ACT[0], "ACT");
//    sc_trace(tracefile, RSTB, "RSTB");
    sc_trace(tracefile, AB_mode[0], "AB_mode");
    sc_trace(tracefile, pim_mode[0], "pim_mode");
    sc_trace(tracefile, bank_addr[0], "bank_addr");
    sc_trace(tracefile, row_addr[0], "row_addr");
    sc_trace(tracefile, col_addr[0], "col_addr");
    sc_trace(tracefile, DQ, "DQ");
    for (i = 0; i < CORES_PER_PCH; i++) {
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->data_out, "data_out");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->ext2crf, "ext2crf");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->ext2srf, "ext2srf");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->ext2grf[0], "ext2grf");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->PC, "PC");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->instr, "instr");
        sc_trace(tracefile, even_buses[0][i], "even_bus");
        sc_trace(tracefile, odd_buses[0][i], "odd_bus");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->even2grfa[0], "even2grfa");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->odd2grfb[0], "odd2grfb");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->crf_wr_en, "crf_wr_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->crf_wr_addr, "crf_wr_addr");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_rd_addr, "srf_rd_addr");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_wr_addr, "srf_wr_addr");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_rd_a_nm, "srf_rd_a_nm");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_wr_a_nm, "srf_wr_a_nm");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_wr_en, "srf_wr_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_wr_from, "srf_wr_from");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->srf_out, "srf_out");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_rd_addr1, "grfa_rd_addr1");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_rd_addr2, "grfa_rd_addr2");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_wr_addr, "grfa_wr_addr");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_wr_en, "grfa_wr_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_relu_en, "grfa_relu_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_wr_from, "grfa_wr_from");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_out1[0], "grfa_out1");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfa_out2[0], "grfa_out2");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_rd_addr1, "grfb_rd_addr1");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_rd_addr2, "grfb_rd_addr2");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_wr_addr, "grfb_wr_addr");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_wr_en, "grfb_wr_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_relu_en, "grfb_relu_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_wr_from, "grfb_wr_from");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_out1[0], "grfb_out1");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->grfb_out2[0], "grfb_out2");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_mult_en, "fpu_mult_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_add_en, "fpu_add_en");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_out_sel, "fpu_out_sel");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_mult_in1_sel, "fpu_mult_in1_sel");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_mult_in2_sel, "fpu_mult_in2_sel");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_add_in1_sel, "fpu_add_in1_sel");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_add_in2_sel, "fpu_add_in2_sel");
        sc_trace(tracefile, dut.imc_pchs[0]->imc_cores[i]->fpu_out[0], "fpu_out");
    }

    sc_start();

    sc_close_vcd_trace_file(tracefile);

    return 0;
}


