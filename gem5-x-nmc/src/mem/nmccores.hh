#ifndef __NMCCORES_HH__
#define __NMCCORES_HH__

#include "sim/sim_object.hh"
#include "params/nmccores.hh"
#include "mem/packet.hh"
#include "base/types.hh"
#include <string>
#include "base/addr_range.hh"

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <assert.h>
#include "base/output.hh"

#include <stdio.h> // for printf
#include <stdlib.h> // for exit()
#include <stdint.h>
#include <errno.h>
// TODO check how to clean this up
#include "../../ext/NMCcores/NMCcores/src/defs.h"
#include "../../ext/NMCcores/NMCcores/src/opcodes.h"

// TODO make this a parameter from cmd line
#define NUM_SIM_CHANNEL 1  // To speed up simulation, the number of simulated CnM channels can be limited

#define ADDR_OFFSET     0x400000000
#define RF_START        (ADDR_OFFSET + (1UL << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS - 1)))
#define CRF_START       (RF_START + (RF_CRF << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define SRFM_START      (RF_START + (RF_SRF_M << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define SRFA_START      (RF_START + (RF_SRF_A << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define GRFA_START      (RF_START + (RF_GRF_A << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define GRFB_START      (RF_START + (RF_GRF_B << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define GRFB_END        (RF_START + ((RF_GRF_B+1UL) << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS)))
#define EXEC_START      ADDR_OFFSET
#define EXEC_END        (RF_START - 1)

// Addresses to change between memory and NMC modes for the different channels
#define MODE_CHANGE_START   (ADDR_OFFSET + ((1UL << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS + BANK_BITS + ROW_BITS)) - 1) - ((1UL << (GLOBAL_OFFSET + CHANNEL_BITS)) - 1))
#define MODE_CHANGE_END     (MODE_CHANGE_START + (((1UL << (CHANNEL_BITS)) - 1) << GLOBAL_OFFSET))

#define AB_MODE 1

//needed masks for AB_MODE
#define MASK_BANK_PARITY    (1UL << GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS)    // Keeps last bank bit
#define MASK_REMOVE_BABG    (~(((1UL << (BG_BITS + BANK_BITS)) - 1) << (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS))) // Sets to 0 bank and bg bits
#define MASK_CHANNEL        ((1UL << CHANNEL_BITS) - 1) << GLOBAL_OFFSET    // Keeps channel bits
#define NUM_BANK            (1 << (BANK_BITS))
#define NUM_BG              (1 << (BG_BITS))
#define NUM_CHANNEL         (1 << (CHANNEL_BITS))
#define SHIFT_CHANNEL       (GLOBAL_OFFSET) // Shifts to the channel bits
#define SHIFT_BG            (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS)   // Shifts to the bg bits
#define SHIFT_BANK          (GLOBAL_OFFSET + CHANNEL_BITS + COL_BITS + RANK_BITS + BG_BITS)   // Shifts to the bank bits

#include "base/callback.hh"

class NMCcores : public SimObject 
{
    private:

        void rcvCnmWriteData(); // Receives the data written by the CnM PUs in exec region, exactly one cycle after the access

        EventFunctionWrapper advanceOneCycle_event;

        uint8_t *pmemAddr_copy;
        uint64_t RangeStart_copy;
        uint8_t *hostAddrBase;

        pid_t pid;
        pid_t gem5_pid; // Used to generate simulation-dependent semaphores and shared memory
        std::string gem5_pid_string;

        sem_t* semaphore1;
        sem_t* semaphore2;

        std::string semName1 = "/semaphoreOne";
        std::string semName2 = "/semaphoreTwo";
        std::string shmName = "/gem5SharedMemory";

        typedef struct FileLine{
            uint64_t address;
            uint64_t dataArray[DWORDS_PER_COL*CORES_PER_PCH];
            uint64_t issuedTick;
            uint64_t simCycle;
            uint8_t nmcMode;
            uint8_t RDcmd;
        } FileLine;

        uint bits_ch;
        uint bits_ra;
        uint bits_bg;
        uint bits_ba;
        uint bits_ro;
        uint bits_co;

        Addr rf_start;
        Addr crf_start;
        Addr srfm_start;
        Addr srfa_start;
        Addr grfa_start;
        Addr grfb_start;
        Addr grfb_end;
        Addr exec_start;
        Addr exec_end;
        
        void copyFileLine(FileLine* dest, FileLine* src);
        void printFileLine(FileLine* fl);

        void* sharedMemPtr;
        FileLine* sharedCnmInfo;
        uint8_t* sharedLastCmd;
        FileLine localCnmInfo[NUM_SIM_CHANNEL];

        uint8_t nmcMode[NUM_SIM_CHANNEL];  // Tracks the NMC mode of each channel

        uint64_t temp[NUM_SIM_CHANNEL][DQ_CLK]; // Stores the column data for each channel
        Addr addr_temp[NUM_SIM_CHANNEL];        // Stores the address of the column data for each channel

        Addr bankParity;
        Addr addrRemoveBABG;

        std::deque<uint> channelCnmWrite;
        
    public:

        typedef NMCcoresParams Params;

        NMCcores(const Params *params);

        void packetInfo(PacketPtr pkt);     //gets the information of the packet and sends it to SystemC, stores and reads in memory depending on the command and region

        void copyhostAddr(uint8_t *hostAddrPart);       //gets the host Address from ramulator, to use it when doing WRs in memory via memcpy

        void copyRangeStart(uint64_t rngStrt);      //gets the Range of memory from ramulaor, to use it when doing WRs in memory via memcpy

        Addr returnAddr(PacketPtr pkt);     //gets the address of the packet

        uint8_t isRDCmd(PacketPtr pkt);        //returns true if the packet has a RD command, used to give this information to SystemC

        uint64_t returnData(PacketPtr pkt);     //returns the data of the packet

        void printSem();        //prints the semaphores

        ~NMCcores();

        void initSharedMemory();

        void endSystemCSim();

        class NMCcoresExitCallback : public Callback
        {
            private:
            NMCcores *nmc; // i think i do not have to do this, because it is done in ramulator for NMCcores
            public:
            virtual ~NMCcoresExitCallback() { } 
            //i dont know exactly what this does
            NMCcoresExitCallback(NMCcores *_nmc)
            {
                nmc = _nmc;
            }
    
            virtual void
            process();
        };

        NMCcoresExitCallback *NMCcoresExitCallback;
};


#endif // __NMCCORES_HH__

