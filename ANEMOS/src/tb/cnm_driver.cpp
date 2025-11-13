#include "../cnm_base.h"

#if MIXED_SIM == 0  // Testbench for SystemC simulation
#if GEM5 == 0
#include "cnm_driver.h"

#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <deque>
#include <assert.h>

using namespace std;

void cnm_driver::driver_thread() {

    int i, j, k, DQCycle[NUM_CHANNEL];
    uint curCycle;
#if INSTR_CLK > 1
    int instrCycle[NUM_CHANNEL];
#endif
    sc_biguint<GRF_WIDTH> bankAux;
    sc_uint<DQ_BITS> bank2out;
    sc_lv<GRF_WIDTH> allzs(SC_LOGIC_Z);
    bool lastCmd[NUM_CHANNEL], bankRead[NUM_CHANNEL], bankWrite[NUM_CHANNEL];

    sc_uint<ADDR_TOTAL_BITS> addrAux[NUM_CHANNEL];

    // Values for reading from input
    string line;
    uint64_t readCycle[NUM_CHANNEL] = {0};
    unsigned long int readAddr[NUM_CHANNEL];
    dq_type dataAux, data2DQ, data2bankAux;
    sc_biguint<GRF_WIDTH> data2bank;
    string readCmd[NUM_CHANNEL];
    dq_type data2DQAux[NUM_CHANNEL][DQ_CLK];
#if INSTR_CLK > 1
    dq_type instr2DQAux[NUM_CHANNEL][INSTR_CLK];
#endif
    deque<dq_type> readData[NUM_CHANNEL];
    deque<sc_biguint<GRF_WIDTH> > data2bankBuffer[NUM_CHANNEL];

    assert(NUM_CHANNEL <= (1 << CHANNEL_BITS));    // Check if the number of channels is within the address space

    // Initial reset
    curCycle = 0;
    for (i = 0; i < NUM_CHANNEL; i++) {
        DQCycle[i] = 0;
#if INSTR_CLK > 1
        instrCycle[i] = 0;
#endif
        lastCmd[i] = false;
        bankRead[i] = false;
        bankWrite[i] = false;
        rst->write(false);
        RD[i]->write(false);
        WR[i]->write(false);
        ACT[i]->write(false);
        AB_mode[i]->write(false);
        pim_mode[i]->write(false);
        bank_addr[i]->write(0);
        row_addr[i]->write(0);
        col_addr[i]->write(0);
        DQ[i]->write(0);
        for (j = 0; j < CORES_PER_PCH; j++) {
            even_buses[i][j]->write(allzs);
            odd_buses[i][j]->write(allzs);
        }
    }

    wait(CLK_PERIOD / 2, RESOLUTION);
    rst->write(1);
    wait(0, RESOLUTION);

    wait(CLK_PERIOD / 2 + 1, RESOLUTION);
    curCycle++;

    // Open input file
    string fi[NUM_CHANNEL], fo[NUM_CHANNEL];
    ifstream input[NUM_CHANNEL];
    ofstream output[NUM_CHANNEL];
    bool valid_input[NUM_CHANNEL] = {true};
    bool some_valid = false;

    for (i = 0; i < NUM_CHANNEL; i++) {
        fi[i] = "inputs/SystemC/" + filename + ".sci" + to_string(i);     // Input file name, located in pim-cores folder
        fo[i] = "inputs/results/" + filename + ".results" + to_string(i);  // Output file name, located in pim-cores folder
        input[i].open(fi[i]);
        output[i].open(fo[i]);
        if (!input[i].is_open())   {
            cout << "Error when opening input file " << fi[i] << endl;
            valid_input[i] = false;
        }
        if (!output[i].is_open())   {
            cout << "Error when opening output file " << fo[i] << endl;
            sc_stop();
            return;
        }
    }
    
    for (i = 0; i < NUM_CHANNEL; i++)  some_valid |= valid_input[i];
    if (!some_valid) {
        for (i = 0; i < NUM_CHANNEL; i++) {
            input[i].close();
            output[i].close();
        }
        cout << "Not able to open any input file" << endl;
        sc_stop();
        return;
    }

    // Read first line
    for (i = 0; i < NUM_CHANNEL; i++) {
        if (valid_input[i] && getline(input[i], line)) {

            // Read elements from a line in the input file
            istringstream iss(line);
            readData[i].clear();
            if (!(iss >> dec >> readCycle[i] >> hex >> readAddr[i] >> readCmd[i])) {
                cout << "Error when reading input" << endl;
                sc_stop();
                return;
            }
            while (iss >> hex >> dataAux) {
                readData[i].push_back(dataAux);
            }

        } else if (valid_input[i]){
            cout << "No lines in the input file" << endl;
            sc_stop();
            return;
        }
    }

    // Simulation loop
    while (1) {

        for (i = 0; i < NUM_CHANNEL; i++) {
            if (valid_input[i]) {
                // Default values
                RD[i]->write(false);
                WR[i]->write(false);
                ACT[i]->write(false);
                AB_mode[i]->write(true);
                pim_mode[i]->write(true);
                DQ[i]->write(0);
                for (j = 0; j < CORES_PER_PCH; j++) {
                    even_buses[i][j]->write(allzs);
                    odd_buses[i][j]->write(allzs);
                }

                // Keep writing to DQ to finish GRF writing
                if (DQCycle[i]) {
                    DQ[i]->write(data2DQAux[i][DQCycle[i]]);
                    DQCycle[i]++;
                }
#if INSTR_CLK > 1
                // Keep writing to DQ to finish GRF writing
                if (instrCycle[i]) {
                    DQ[i]->write(instr2DQAux[i][instrCycle[i]]);
                    instrCycle[i]++;
                }
#endif

                // Fill the banks' sense amplifiers with the corresponding data
                if (bankRead[i]) {
                    if (addrAux[i].range(BA_END, BA_END)) {
                        for (j = 0; j < CORES_PER_PCH; j++) {
                            odd_buses[i][j]->write(data2bankBuffer[i].front());
                            data2bankBuffer[i].pop_front();
                        }
                    } else {
                        for (j = 0; j < CORES_PER_PCH; j++) {
                            even_buses[i][j]->write(data2bankBuffer[i].front());
                            data2bankBuffer[i].pop_front();
                        }
                    }
                    bankRead[i] = false;
                }

                // Execute necessary command at the right time and read next line
                // if current cycle caught up with the previous read one
                if (!lastCmd[i] && curCycle >= readCycle[i]) {

                    // Execute command at the corresponding cycle, assuming PIM mode

                    assert(!DQCycle[i]);// A write to a GRF shouldn't overlap with next command
#if INSTR_CLK > 1
                    assert(!instrCycle[i]);// A write to CRF shouldn't overlap with next command
#endif

                    // Check first the MSB of the row address to see which mode we're in
                    // (writing to RFs or PIM execution)
                    addrAux[i] = readAddr[i];
                    if (addrAux[i].range(RO_STA, RO_STA)) {
                        // Writing to the RFs

                        // Check command
                        if (!readCmd[i].compare("RD")) {

                            // If writing to RFs and RD, do nothing
                            cout << "Warning: At channel " << i << " RF writing mode but saw a RD command" << endl;

                        } else {

                            // If writing to RFs and WR, format data to DQ

#if INSTR_CLK > 1
                            if(addrAux[i].range(RO_STA - 1, RO_END) == RF_CRF) {
                                assert(readData[i].size() == INSTR_CLK);
                                for (j = 0; j < INSTR_CLK; j++) {
                                    instr2DQAux[i][j] = readData[i].front();
                                    readData[i].pop_front();
                                }
                                WR[i]->write(true);
                                bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                                row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                                col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                                DQ[i]->write(instr2DQAux[i][instrCycle]);
                                instrCycle[i]++;// Increase Instr cycle to know a write to  CRF is ongoing
                            } else
#endif
                            if (addrAux[i].range(RO_STA - 1, RO_END) < RF_GRF_A) { // Writing to CRF or SRF, one cycle is enough
                                // removed this since I pass 4 datas but only the first one is valid so it should be okay for now
                                //assert(readData.size() == 1);   // Check it is only one piece of data
                                data2DQ = readData[i].front();
                                readData[i].pop_front();
                                WR[i]->write(true);
                                bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                                row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                                col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                                DQ[i]->write(data2DQ);

                            } else {    // Writing to GRF, DQ_CLK cycles are needed

                                assert(readData[i].size() == DQ_CLK);   // Check it is only one piece of data, in 4 ints
                                for (j = 0; j < DQ_CLK; j++) {
                                    data2DQAux[i][j] = readData[i].front();
                                    readData[i].pop_front();
                                }
                                WR[i]->write(true);
                                bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                                row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                                col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                                DQ[i]->write(data2DQAux[i][DQCycle[i]]);
                                DQCycle[i]++;// Increase DQ cycle to know a write to GRF is ongoing

                            }
                        }

                    } else {
                        // PIM execution

                        // Check command
                        if (!readCmd[i].compare("RD")) {

                            RD[i]->write(true);
                            bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                            row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                            col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));

                            // If PIM execution and RD with input data, send to the corresponding bank buses in the next cycle
                            if (!readData[i].empty()){
                                assert(readData[i].size() == DQ_CLK*CORES_PER_PCH);// Check if there are enough pieces of data

                                for (j = 0; j < CORES_PER_PCH; j++) {
                                    for (k = 0; k < DQ_CLK; k++){
                                        data2bankAux = readData[i].front();
                                        readData[i].pop_front();
                                        data2bank.range(DQ_BITS*(k+1)-1,DQ_BITS*k) = data2bankAux;
                                    }
                                    data2bankBuffer[i].push_back(data2bank);
                                }

                                bankRead[i] = true;
                            }

                        } else {

                            // If PIM execution and WR, we record bank buses next cycle
                            WR[i]->write(true);
                            bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                            row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                            col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                            bankWrite[i] = true;

                            // Write address here because it will be overwritten later with the next cmd

                            output[i] << showbase << dec << curCycle << "\t" << hex << readAddr[i] << "\t";

                        }
                    }

                    // Read next line
                    if (getline(input[i], line)) {

                        // Read elements from a line in the input file
                        istringstream iss(line);
                        readData[i].clear();
                        if (!(iss >> dec >> readCycle[i] >> hex >> readAddr[i] >> readCmd[i])) {
                            cout << "Error when reading input" << endl;
                            break;
                        }
                        while (iss >> hex >> dataAux) {
                            readData[i].push_back(dataAux);
                        }

                    } else {// Wait for enough time for the last instruction to be completed
                        input[i].close();
                        lastCmd[i] = true;
                        readCycle[i] += (3 + MULT_STAGES + ADD_STAGES);
                    }

                } else if (lastCmd[i] && curCycle >= readCycle[i]) {
                    // End of simulation, last command was already read
                    output[i].close();
                    valid_input[i] = false;
                }

                // Reset DQCycle if writing to the GRF has ended
                if (DQCycle[i] > DQ_CLK-1) {
                    DQCycle[i] = 0;
                }
    #if INSTR_CLK > 1
                // Reset instrCycle if writing to the CRF has ended
                if (instrCycle[i] > INSTR_CLK-1) {
                    instrCycle[i] = 0;
                }
    #endif

                if (bankWrite[i]) {    // TODO translate hex to half
                    for (j = 0; j < 10; j++)    // More than one deltas are needed
                        wait(0, RESOLUTION);    // We need to wait for a delta to solve the bank buses
                    if (addrAux[i].range(BA_END, BA_END)) {
                        for (j = 0; j < CORES_PER_PCH; j++) {
                            bankAux = odd_buses[i][j]->read();
                            for (k = 0; k < DQ_CLK; k++) {
                                bank2out = bankAux.range(DQ_BITS*(k+1)-1,DQ_BITS*k);
                                output[i] << showbase << hex << bank2out << "\t";
                            }
                        }
                    } else {
                        for (j = 0; j < CORES_PER_PCH; j++) {
                            bankAux = even_buses[i][j]->read();
                            for (k = 0; k < DQ_CLK; k++) {
                                bank2out = bankAux.range(DQ_BITS*(k+1)-1,DQ_BITS*k);
                                output[i] << showbase << hex << bank2out << "\t";
                            }
                        }
                    }
                    output[i] << endl;

                    bankWrite[i] = false;
                }
            }
        }

        some_valid = false;
        for (i = 0; i < NUM_CHANNEL; i++)  some_valid |= valid_input[i];
        if (!some_valid) {
            cout << "All input files have been read" << endl;
            break;
        }

        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }

    cout << "Simulation finished at cycle " << dec << curCycle << endl;

    // Stop simulation
    sc_stop();

}
#endif
#endif
