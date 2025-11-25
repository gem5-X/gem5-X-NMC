#include "../cnm_base.h"

#if MIXED_SIM == 0  // Testbench for SystemC simulation
#if GEM5 == 1
#include "cnm_driver.h"

#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <deque>
#include <assert.h>

using namespace std;

void cnm_driver::copyFileLine (FileLine* dest, FileLine* src) {
    dest->address = src->address;
    for(int j = 0; j < DWORDS_PER_COL*CORES_PER_PCH; j++){
        dest->dataArray[j] = src->dataArray[j];
    }
    dest->issuedTick = src->issuedTick;
    dest->simCycle = src->simCycle;
    dest->pimMode = src->pimMode;
    dest->RDcmd = src->RDcmd;
}

void cnm_driver::copySharedMem (FileLine* destFileLine, uint8_t* destLastCmd,
                                FileLine* srcFileLine, uint8_t* srcLastCmd) {
    for (int i = 0; i < NUM_CHANNEL; i++) {
        destFileLine[i].address = srcFileLine[i].address;
        for(int j = 0; j < DWORDS_PER_COL*CORES_PER_PCH; j++){
            destFileLine[i].dataArray[j] = srcFileLine[i].dataArray[j];
        }
        destFileLine[i].issuedTick = srcFileLine[i].issuedTick;
        destFileLine[i].simCycle = srcFileLine[i].simCycle;
        destFileLine[i].pimMode = srcFileLine[i].pimMode;
        destFileLine[i].RDcmd = srcFileLine[i].RDcmd;
    }
    *destLastCmd = *srcLastCmd;    
}

void cnm_driver::printFileLine (FileLine* fl) {
    cout << "Addr " << hex << showbase << fl->address << " data[0] " << fl->dataArray[0];
    cout << " Tick " << dec << fl->issuedTick << " Eq. cycle " << fl->simCycle << " pimMode " << uint(fl->pimMode) << " RDcmd " << uint(fl->RDcmd) << endl;
}

void cnm_driver::printSharedMem (FileLine* fl, uint8_t* lastCmdPtr) {
    cout << "LastCmd: " << uint(*lastCmdPtr) << endl;
    for (int i = 0; i < NUM_CHANNEL; i++) {
    	std::cout << "fl[i].pimMode" << uint(fl[i].pimMode) << std::endl;
        if (fl[i].pimMode == 1) {
            cout << "Channel " << dec << i << " addr " << hex << showbase << fl[i].address << " data[0] " << fl[i].dataArray[0];
            cout << " Tick " << dec << fl[i].issuedTick << " Eq. cycle " << fl[i].simCycle << " pimMode " << uint(fl[i].pimMode) << " RDcmd " << uint(fl[i].RDcmd) << endl;
        }
    }
    cout << "---------------" << endl;
}

void cnm_driver::driver_thread() {

    int i, j, k, DQCycle[NUM_CHANNEL];
    uint64_t curCycle;
#if INSTR_CLK > 1
    int instrCycle[NUM_CHANNEL];
#endif
    sc_biguint<GRF_WIDTH> bankAux;
    sc_uint<DQ_BITS> bank2out;
    sc_lv<GRF_WIDTH> allzs(SC_LOGIC_Z);
    bool bankRead[NUM_CHANNEL], bankWrite[NUM_CHANNEL];

    sc_uint<ADDR_TOTAL_BITS> addrAux[NUM_CHANNEL];

    // Values for reading from input
    string line;
    uint8_t localPimMode[NUM_CHANNEL] = {0};
    uint64_t readCycle[NUM_CHANNEL] = {0};
    unsigned long int readAddr[NUM_CHANNEL];
    dq_type data2DQ, data2bankAux;
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

    bool rcvNewCmd = true;      // Indicates if we will receive a new command from gem5, or if we just simulate until the next write to the bank
    bool waitSemaphore = true;  // Indicates if we should wait for the semaphore to be posted
    bool writeSync = false;     // Indicates that we have to simulate all the CnM to synchronize with the next write to the bank

    uint64_t tickOffset = 0;        // We only update every time we notice the a new tick is too far ahead from the most recent tick 
    uint64_t mostRecentTick = 0;    // Keep track of the most recent tick

    shmName.append(gem5_pid_string);
    semName1.append(gem5_pid_string);
    semName2.append(gem5_pid_string);

    // Shared memory
    int shm_fd = shm_open(shmName.c_str(), O_RDWR, 0666);
    if(shm_fd == -1){
        perror("Error with shm_open");
        cout << "shmName: " << shmName << endl;
        exit(1);
    }

    void* sharedMemPtr  = mmap(0, NUM_CHANNEL*sizeof(FileLine) + sizeof(uint8_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedMemPtr == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    FileLine* sharedCnmInfo = (FileLine*) sharedMemPtr;
    uint8_t* sharedLastCmd = ((uint8_t*) sharedMemPtr) + NUM_CHANNEL*sizeof(FileLine);
    deque<FileLine> instructionList[NUM_CHANNEL];

    // Semaphores
    sem_t* semaphore1 = sem_open(semName1.c_str(), 1);
    sem_t* semaphore2 = sem_open(semName2.c_str(), 1);

    if (semaphore1 == SEM_FAILED) {
        perror("sem_open/sem1");
        exit(EXIT_FAILURE);
    }
    if (semaphore2 == SEM_FAILED) {
        perror("sem_open/sem2");
        exit(EXIT_FAILURE);
    }


    wait(CLK_PERIOD / 2, RESOLUTION);
    rst->write(1);
    wait(0, RESOLUTION);

    wait(CLK_PERIOD / 2 + 1, RESOLUTION);
    curCycle++;

#if OUTPUT_LOG
    // Open output file
    string fo = filename;   // Output file name, located in pim-cores folder
    ofstream output[NUM_CHANNEL];
    for (i = 0; i < NUM_CHANNEL; i++) {
        output[i].open(fo + to_string(i));
        if (!output[i].is_open())   {
            cout << "Error when opening output file" << endl;
            sc_stop();
            return;
        }
    }
#endif

    FileLine rcvCnmInfo[NUM_CHANNEL];
    FileLine sendCnmInfo[NUM_CHANNEL];
    uint8_t localLastCmd = 0;

    // Simulation loop
    while (1) {

        // Setting default values, write to DQ and read from the buses to the PU.
        // For each channel, before semaphore synchronization
        
        for (i = 0; i < NUM_CHANNEL; i++) {
            
            // Default values
            RD[i]->write(false);
            WR[i]->write(false);
            ACT[i]->write(false);
            AB_mode[i]->write(true);
            pim_mode[i]->write(localPimMode[i]);    // Speed-up simulation of channels in memory mode
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
        }

        if (rcvNewCmd) {
            rcvNewCmd = false;
            if (waitSemaphore) {  // Receiving commands from gem5 // NOTE assuming gem5 can only synch once per cycle
                sem_wait(semaphore1);
                copySharedMem(rcvCnmInfo, &localLastCmd, sharedCnmInfo, sharedLastCmd);
 #ifdef DBGPRINTS
                std::cout << "receive from gem5" << std::endl;
                printSharedMem(rcvCnmInfo, &localLastCmd);
                std::cout << std::endl;
 #endif
                if (localLastCmd) {
                    // Give time to finish the last command
                    for (i = 0; i < NUM_CHANNEL; i++) { readCycle[i] += (3 + MULT_STAGES + ADD_STAGES); }
                } else {
                    for (i = 0; i < NUM_CHANNEL; i++) {
                        // Push command to the list if it is a PIM command and it is new
                        if (rcvCnmInfo[i].pimMode && ((rcvCnmInfo[i].issuedTick-tickOffset)/CLK_PERIOD > curCycle) &&
                            (instructionList[i].empty() || rcvCnmInfo[i].issuedTick != instructionList[i].back().issuedTick)) {
                            // If the tick is too far ahead, update tickOffset, making sure new ticks aren't offset further back
                            // than the most recent tick
                            if (rcvCnmInfo[i].issuedTick > mostRecentTick + TICK_THRESHOLD) {
                                tickOffset += rcvCnmInfo[i].issuedTick - mostRecentTick - TICK_BACKUP;
 #ifdef DBGPRINTS
                                 std::cout << "Tick offset updated to " << dec << tickOffset << std::endl;
 #endif
                            }
                            mostRecentTick = rcvCnmInfo[i].issuedTick;
                            rcvCnmInfo[i].simCycle = (rcvCnmInfo[i].issuedTick - tickOffset) / CLK_PERIOD; 
                            instructionList[i].push_back(rcvCnmInfo[i]);
 #ifdef DBGPRINTS
                             std::cout << "Pushing instruction to the list, current cycle: " << curCycle << " instruction cycle: " << rcvCnmInfo[i].simCycle << std::endl;
 #endif
                        }
                        localPimMode[i] = rcvCnmInfo[i].pimMode;
                    }
                }
                // Check if we need to simulate until the next write to the bank
                writeSync = false;
                for (i = 0; i < NUM_CHANNEL; i++) {
                    writeSync |= (rcvCnmInfo[i].RDcmd == 0) && (rcvCnmInfo[i].address < RF_START);
                }
            }

            // Received command is a read or a write to RF, so we don't simulate CnM and wait for next command
            if (!writeSync && !localLastCmd){  
                rcvNewCmd = true;
                sem_post(semaphore2);
                continue;

            // Received command is a write to the bank, so we simulate everything so far
            } else if (!localLastCmd) {
                if (waitSemaphore) {
                    sem_post(semaphore2);
                    sem_wait(semaphore1);
                    waitSemaphore = false;
                }

                for (i = 0; i < NUM_CHANNEL; i++) {
                    // Read next instructions if caught up with the previous one
                    if (!instructionList[i].empty() && curCycle >= readCycle[i]) {
                        copyFileLine(&sendCnmInfo[i], &instructionList[i].front());
                        instructionList[i].pop_front();
                        readAddr[i] = sendCnmInfo[i].address;
                        addrAux[i] = readAddr[i];
                        readCmd[i] = (sendCnmInfo[i].RDcmd == 1) ? "RD" : "WR";
                        readCycle[i] = sendCnmInfo[i].simCycle;
                        readData[i].clear();
                        if (addrAux[i].range(RO_STA, RO_STA)) {    // Writing to RFs
#if INSTR_CLK > 1
                            if (addrAux[i].range(RO_STA-1, RO_END) == RF_CRF) {
                                for (int j = 0; j < INSTR_CLK; j++) {
                                    readData[i].push_back(sendCnmInfo[i].dataArray[j]);
                                }
                            } else
#endif
                            if (addrAux[i].range(RO_STA-1, RO_END) < RF_GRF_A) {    // CRF or SRF
                                readData[i].push_back(sendCnmInfo[i].dataArray[0]);
                            } else {    // GRFs
                                for (int j = 0; j < DQ_CLK; j++) {
                                    readData[i].push_back(sendCnmInfo[i].dataArray[j]);
                                }
                            }
                        } else {
                            for (int j = 0; j < CORES_PER_PCH; j++) {
                                for (int k = 0; k < DWORDS_PER_COL; k++) {
                                    readData[i].push_back(sendCnmInfo[i].dataArray[j*DWORDS_PER_COL+k]);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Execute necessary command at the right time and read next line
        // if current cycle caught up with the previous read one in one of the channels
        bool caughtUp = false; 
        for (i = 0; i < NUM_CHANNEL; i++) {
            caughtUp |= (localPimMode[i] && curCycle >= readCycle[i]);
        }
        if (!localLastCmd && caughtUp) {
            rcvNewCmd = true;
            // Execute command at the corresponding cycle, assuming PIM mode

            for (i = 0; i < NUM_CHANNEL; i++) {
                if (curCycle == readCycle[i]) { // Only execute if we have a command to execute in this cycle
                    assert(!DQCycle[i]);    // A write to a GRF shouldn't overlap with next command
#if INSTR_CLK > 1
                    assert(!instrCycle[i]); // A write to CRF shouldn't overlap with next command
#endif
                    // Check first the MSB of the row address to see which mode we're in
                    // (writing to RFs or PIM execution)
                    addrAux[i] = readAddr[i];
                    if (addrAux[i].range(RO_STA, RO_STA)) {
                        // Writing to the RFs

                        // Check command
                        if (!readCmd[i].compare("RD")) {
                            // If writing to RFs and RD, do nothing
                            cout << "Warning: RF writing mode but saw a RD command" << endl;
                        } else {
                            // If writing to RFs and WR, format data to DQ
#if INSTR_CLK > 1
                            if (addrAux[i].range(RO_STA - 1, RO_END) == RF_CRF) {
                                assert(readData[i].size() == INSTR_CLK);
                                for (j = 0; j < INSTR_CLK; j++) {
                                    instr2DQAux[i][j] = readData[i].front();
                                    readData[i].pop_front();
                                }
                                WR[i]->write(true);
                                bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                                row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                                col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                                DQ[i]->write(instr2DQAux[i][instrCycle[i]]);
                                instrCycle[i]++;// Increase Instr cycle to know a write to  CRF is ongoing
                            } else
#endif
                            if (addrAux[i].range(RO_STA - 1, RO_END) < RF_GRF_A) { // Writing to CRF or SRF, one cycle is enough
                                assert(readData[i].size() == 1);   // Check it is only one piece of data
                                data2DQ = readData[i].front();
                                readData[i].pop_front();
                                WR[i]->write(true);
                                bank_addr[i]->write(addrAux[i].range(BA_STA, BA_END));
                                row_addr[i]->write(addrAux[i].range(RO_STA, RO_END));
                                col_addr[i]->write(addrAux[i].range(CO_STA, CO_END));
                                DQ[i]->write(data2DQ);

                            } else {    // Writing to GRF, DQ_CLK cycles are needed

                                assert(readData[i].size() == DQ_CLK);  // Check if it fill a GRF
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
                                    for (k = 0; k < DWORDS_PER_COL; k++){
                                        data2bankAux = readData[i].front();
                                        readData[i].pop_front();
                                        data2bank.range(DQ_BITS*(k+1)-1,DQ_BITS*k) = data2bankAux;
                                    }
#ifdef DBGPRINTS
                                    cout << "parsed data to bank: " << hex << data2bank << endl;
#endif
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

#if OUTPUT_LOG
                            // Write address here because it will be overwritten later with the next cmd
                            output[i] << showbase << dec << curCycle << "\t" << hex << readAddr[i] << "\t";
#endif

                        }
                    }
                }
            }
        } else if (localLastCmd && caughtUp) {
            // End of simulation, last command was already read
            munmap(sharedMemPtr, NUM_CHANNEL*sizeof(FileLine) + sizeof(uint8_t));
            shm_unlink(shmName.c_str());
            sem_post(semaphore2);
#if OUTPUT_LOG
            for (i = 0; i < NUM_CHANNEL; i++) { output[i].close(); }
#endif
            break;
        }

        // Finish DQ writing and write from the PU to the buses.
        // For each channel, after semaphore synchronization
        for (i = 0; i < NUM_CHANNEL; i++) {
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
                        for (k = 0; k < DWORDS_PER_COL; k++) {
                            bank2out = bankAux.range(DQ_BITS*(k+1)-1,DQ_BITS*k);
                            sendCnmInfo[i].dataArray[DWORDS_PER_COL*j+k] = bank2out;
#if OUTPUT_LOG
                            output[i] << showbase << hex << bank2out << "\t";
#endif
                        }
                    }
                } else {
                    for (j = 0; j < CORES_PER_PCH; j++) {
                        bankAux = even_buses[i][j]->read();
                        for (k = 0; k < DWORDS_PER_COL; k++) {
                            bank2out = bankAux.range(DQ_BITS*(k+1)-1,DQ_BITS*k);
                            sendCnmInfo[i].dataArray[DWORDS_PER_COL*j+k] = bank2out;
#if OUTPUT_LOG
                            output[i] << showbase << hex << bank2out << "\t";
#endif
                        }
                    }
                }
                copyFileLine(&sharedCnmInfo[i], &sendCnmInfo[i]);
 #ifdef DBGPRINTS
                std::cout << "send to gem5" << std::endl;
                printFileLine(&sendCnmInfo[i]);
                std::cout << std::endl;
 #endif

#if OUTPUT_LOG
                output[i] << endl;
#endif

                bankWrite[i] = false;
            }
        }

        // If all instructions have been executed, wait for the next command
        bool instrListsEmpty = true;
        for (i = 0; i < NUM_CHANNEL; i++) {
            instrListsEmpty &= instructionList[i].empty();
        }
        if(rcvNewCmd && instrListsEmpty){
            waitSemaphore = true;
            sem_post(semaphore2);
        }
        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }

    cout << "Simulation finished at cycle " << dec << curCycle << endl;

    // Stop simulation
    sc_stop();

}

#endif  // GEM5
#endif  // MIXED_SIM
