#include "systemc.h"
#include "../cnm_base.h"

#if GEM5
    //Needed libraries for semaphores/shared memory (testbench stuff)
    #include <iostream>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sstream>
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <semaphore.h>
    #include <assert.h>
    #include <cstdio>
    #include <cstdlib>
    #include <stdint.h>
    #include <fstream>
    #include <string>
    #define RF_START    (1UL << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS - 1))
    #define TICK_THRESHOLD  (1 << 20)
    #define TICK_BACKUP     20000 // changed 10k to 20k, possible reason for the bug
#endif

class cnm_driver: public sc_module {
public:

    sc_out<bool>                rst;
    sc_out<bool>                RD[NUM_CHANNEL];                         // DRAM read command
    sc_out<bool>                WR[NUM_CHANNEL];                         // DRAM write command
    sc_out<bool>                ACT[NUM_CHANNEL];                        // DRAM activate command
//    sc_out<bool>              RSTB[NUM_CHANNEL];                       //
    sc_out<bool>                AB_mode[NUM_CHANNEL];                    // Signals if the All-Banks mode is enabled
    sc_out<bool>                pim_mode[NUM_CHANNEL];                   // Signals if the PIM mode is enabled
    sc_out<sc_uint<BANK_BITS> > bank_addr[NUM_CHANNEL];                  // Address of the bank
    sc_out<sc_uint<ROW_BITS> >  row_addr[NUM_CHANNEL];                   // Address of the bank row
    sc_out<sc_uint<COL_BITS> >  col_addr[NUM_CHANNEL];                   // Address of the bank column
    sc_out<sc_uint<DQ_BITS> >   DQ[NUM_CHANNEL];                         // Data input from DRAM controller (output makes no sense)
    sc_inout_rv<GRF_WIDTH>      even_buses[NUM_CHANNEL][CORES_PER_PCH];  // Direct data in/out to the even banks
    sc_inout_rv<GRF_WIDTH>      odd_buses[NUM_CHANNEL][CORES_PER_PCH];   // Direct data in/out to the odd banks

#if GEM5
    //Communicating struct with gem5
    typedef struct FileLine{
        uint64_t address;
        uint64_t dataArray[DWORDS_PER_COL*CORES_PER_PCH];
        uint64_t issuedTick;
        uint64_t simCycle;
        uint8_t pimMode;
        uint8_t RDcmd;
    } FileLine;

    void copyFileLine(FileLine* dest, FileLine* src);
    void copySharedMem(FileLine* destFileLine, uint8_t* destLastCmd,
                        FileLine* srcFileLine, uint8_t* srcLastCmd);
    void printFileLine(FileLine* fl);
    void printSharedMem(FileLine* fl, uint8_t* lastCmdPtr);

    //Shared memory and semaphores
    std::string semName1 = "/semaphoreOne";
    std::string semName2 = "/semaphoreTwo";
    std::string shmName = "/gem5SharedMemory";

#endif

    std::string filename;

#if (GEM5)
    std::string gem5_pid_string;

    SC_HAS_PROCESS(cnm_driver);
    cnm_driver(sc_module_name name_, std::string filename_, std::string _pid) :
        sc_module(name_), filename(filename_), gem5_pid_string(_pid) {
            SC_THREAD(driver_thread);
    }
#else
    SC_HAS_PROCESS(cnm_driver);
    cnm_driver(sc_module_name name_, std::string filename_) : sc_module(name_), filename(filename_) {
        SC_THREAD(driver_thread);
    }
#endif

    void driver_thread();
};
