#include "mem/nmccores.hh"
#include <iostream>
//#include <libexplain/execvp.h>

NMCcores::NMCcores(const Params *params):
    SimObject(params),
    advanceOneCycle_event([this]{rcvCnmWriteData();}, name()),
    pmemAddr_copy(nullptr), RangeStart_copy(0), hostAddrBase(nullptr),
    pid(0),
    gem5_pid(0),
    semaphore1(0),
    semaphore2(0),
    sharedMemPtr(nullptr),
    sharedCnmInfo(nullptr),
    sharedLastCmd(nullptr),
    bankParity(0), addrRemoveBABG(0)
{
    // Generate simulation-independent semaphore and shared memory names
    gem5_pid = getpid();
    gem5_pid_string = "." + std::to_string(gem5_pid);
    shmName.append(gem5_pid_string);
    semName1.append(gem5_pid_string);
    semName2.append(gem5_pid_string);

    int shm_fd = shm_open(shmName.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);
    while(shm_fd<0){
        // Shared memory already exists, new identifier generated for it
        gem5_pid++;
        gem5_pid_string = "." + std::to_string(gem5_pid);

        shmName.replace(shmName.begin()+17,shmName.end(),gem5_pid_string);  // 17 = nr of characters in shmName = "/gem5SharedMemory" 
        semName1.replace(semName1.begin()+13,semName1.end(),gem5_pid_string);  // 13 = nr of characters in semName1 = "/semaphoreOne";
        semName2.replace(semName2.begin()+13,semName2.end(),gem5_pid_string);  // 13 = nr of characters in semName2 = "/semaphoreTwo";

        shm_fd = shm_open(shmName.c_str(), O_CREAT | O_RDWR| O_EXCL, 0666);
    }
    if (shm_fd == -1) {
        perror("Cannot open shared memory descriptor");
    }

    sharedMemPtr = (void*) mmap(NULL, NUM_SIM_CHANNEL*sizeof(FileLine) + sizeof(uint8_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedMemPtr == MAP_FAILED) {
	    printf("error is %d\n", errno);
	    perror("Cannot MAP");
    } else { 
        std::cout << "pointer of mapped region = " << sharedMemPtr << std::endl;
    }
    sharedCnmInfo = (FileLine*) sharedMemPtr;
    sharedLastCmd = ((uint8_t*) sharedMemPtr) + NUM_SIM_CHANNEL*sizeof(FileLine);

    int result = ftruncate(shm_fd, NUM_SIM_CHANNEL*sizeof(FileLine) + sizeof(uint8_t));
    
    initSharedMemory();

    if (sem_unlink(semName1.c_str()) == 0) {
        std::cout << "Semaphore unlinked successfully." << std::endl;
    } else {
        perror("sem_unlink failed");
    }
    if (sem_unlink(semName2.c_str()) == 0) {
        std::cout << "Semaphore unlinked successfully." << std::endl;
    } else {
        perror("sem_unlink failed");
    }

    semaphore1 = sem_open(semName1.c_str(), O_CREAT, 0666, 0);
    semaphore2 = sem_open(semName2.c_str(), O_CREAT, 0666, 0);
    if (semaphore1 == SEM_FAILED) {
        std::cout << "sem failed" << std::endl;
    }
    if (semaphore2 == SEM_FAILED) {
        std::cout << "sem failed" << std::endl;
    }

    pid = fork();

    // TODO automate this
    if (pid == 0) {
        std::string scPath = simout.resolve("SystemC" + gem5_pid_string + ".results");
        execl("/CrossLayerNMC/gem5-x-nmc/ext/NMCcores/pim-cores-HBM-NEW", 
              "/CrossLayerNMC/gem5-x-nmc/ext/NMCcores/pim-cores-HBM-NEW", scPath.c_str(), gem5_pid_string.c_str(),  nullptr);
        std::cout << "error with execl" << std::endl;
    } else {
        std::cout << "==============================================================================================================" << std::endl;
        std::cout << "Started child process with PID " << pid << " ,you might need to manually kill child process if gem5 crashes" << std::endl;
        std::cout << "==============================================================================================================" << std::endl;
    }

    std::cout << "RF_START " << std::hex << RF_START << " MODE_CHANGE_START " << MODE_CHANGE_START << " MODE_CHANGE_END " << MODE_CHANGE_END << std::endl;

    nmccoresExitCallback = new NMCcoresExitCallback(this);
    registerExitCallback(nmccoresExitCallback);
}

NMCcores::~NMCcores()
{
    delete nmccoresExitCallback;
}

void NMCcores::rcvCnmWriteData() {
    sem_post(semaphore1);
    sem_wait(semaphore2);

    uint channel = channelCnmWrite.front();
    channelCnmWrite.pop_front();

    copyFileLine(&localCnmInfo[channel], &sharedCnmInfo[channel]);
    sharedCnmInfo[channel].RDcmd = 1;  // Put back RDcmd = 1 for the specific channel to avoid multiple writes
// TODO maybe add them as DEBUG options
    // std::cout << "receive from SystemC" << std::endl;
    // printFileLine(&localCnmInfo[channel]);
    // std::cout << std::endl;
    // std::cout << "Pointed struct copied to gem5, address" << std::hex << std::showbase << localCnmInfo[channel].address << std::endl;
    hostAddrBase = pmemAddr_copy - RangeStart_copy + ADDR_OFFSET;
#if AB_MODE
    bankParity =(uint64_t) ((uint64_t)localCnmInfo[channel].address & MASK_BANK_PARITY);
    // std::cout << std::hex << "bankParity " << bankParity << std::endl;
    addrRemoveBABG = (uint64_t) ((localCnmInfo[channel].address & MASK_REMOVE_BABG) + (uint64_t) hostAddrBase);

    if (bankParity == 0) {    // Even bank
        for (int i = 0; i < NUM_BG; i++) {  // Loop over Bank Groups
            for (int j = 0; j < NUM_BANK; j+=2) {   // Loop over even banks
                // std::cout << std::showbase << std::hex << "Even address = " << localCnmInfo[channel].address  << "\t" << localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+(j/2))] << std::endl; 
                memcpy((uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK)), &localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+(j/2))], DWORDS_PER_COL*sizeof(uint64_t));
            }
        }
    } else {    // Odd bank
        for (int i = 0; i < NUM_BG; i++) {  // Loop over Bank Groups
            for (int j = 1; j < NUM_BANK; j+=2) {   // Loop over odd banks
                // std::cout << std::showbase << std::hex << "Odd address = " << localCnmInfo[channel].address  << "\t" << localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+((j-1)/2))] << std::endl; 
                memcpy((uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK)), &localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+((j-1)/2))], DWORDS_PER_COL*sizeof(uint64_t));
            }
        }
    }
#else
    memcpy(hostAddrBase+localCnmInfo[channel].address, localCnmInfo[channel].dataArray, DWORDS_PER_COL*CORES_PER_PCH*sizeof(uint64_t));
#endif
}

void NMCcores::copyFileLine (FileLine* dest, FileLine* src) {
    dest->address = src->address;
    for(int j = 0; j < DWORDS_PER_COL*CORES_PER_PCH; j++){
        dest->dataArray[j] = src->dataArray[j];
    }
    dest->issuedTick = src->issuedTick;
    dest->simCycle = src->simCycle;
    dest->nmcMode = src->nmcMode;
    dest->RDcmd = src->RDcmd;
}

void NMCcores::printFileLine (FileLine* fl) {
    std::cout << "Addr " << std::hex << std::showbase << fl->address << " data[0] " << fl->dataArray[0];
    std::cout << " Tick " << std::dec << fl->issuedTick << " simCycle " << fl->simCycle << " nmcMode " << uint(fl->nmcMode) << " RDcmd " << uint(fl->RDcmd) << std::endl;
}

void NMCcores::packetInfo(PacketPtr pkt){

    uint channel = ((pkt->getAddr() && MASK_CHANNEL) >> SHIFT_CHANNEL);
    if (channel >= NUM_SIM_CHANNEL) {
        std::cout << "Channel out of bounds" << std::endl;  // TODO turn into DEBUG message
        return;
    }

    // Check if switching memory mode
    if (pkt->getAddr() >= MODE_CHANGE_START && pkt->getAddr() <= MODE_CHANGE_END) {
        nmcMode[channel] = nmcMode[channel] ? 0 : 1;
        std::cout << "Changed channel " << channel << " to NMC mode " << uint(nmcMode[channel]) << std::endl;
        sharedCnmInfo[channel].nmcMode = nmcMode[channel];
        localCnmInfo[channel].nmcMode = nmcMode[channel];
        return; // Do not process the packet further
    }
    //GRF
    if (nmcMode[channel] && pkt->getAddr() >= GRFA_START && pkt->getAddr() < GRFB_END) {

        uint dqCycle = (pkt->getAddr() % (GRF_WIDTH/8)) / (DQ_BITS / 8);

        if (!dqCycle) {
            addr_temp[channel] = pkt->getAddr();    // Store the address of the first byte of the GRF
        }
        temp[channel][dqCycle] = *(pkt->getConstPtr<uint64_t>());
        
        if(dqCycle == DQ_CLK - 1) {

            localCnmInfo[channel].address = (addr_temp[channel] - ADDR_OFFSET);
            localCnmInfo[channel].RDcmd = pkt->isRead();
            for (int i = 0; i < DQ_CLK; i++) {
                localCnmInfo[channel].dataArray[i] = temp[channel][i];
            }
            localCnmInfo[channel].issuedTick = curTick();
            localCnmInfo[channel].simCycle = 0;

            copyFileLine(&sharedCnmInfo[channel], &localCnmInfo[channel]);
            // std::cout << "send to SystemC" << std::endl;
            // printFileLine(&localCnmInfo[channel]);
            // std::cout << std::endl;
            sem_post(semaphore1);
            sem_wait(semaphore2);
        }
    //SRF and CRF
    } else if (nmcMode[channel] && pkt->getAddr() >= CRF_START && pkt->getAddr() < GRFA_START) {
        localCnmInfo[channel].address = pkt->getAddr() - ADDR_OFFSET;
        localCnmInfo[channel].RDcmd = pkt->isRead();
        localCnmInfo[channel].dataArray[0] = *(pkt->getConstPtr<uint64_t>());
        localCnmInfo[channel].issuedTick = curTick();
        localCnmInfo[channel].simCycle = 0;

        copyFileLine(&sharedCnmInfo[channel], &localCnmInfo[channel]);
        // std::cout << "send to SystemC" << std::endl;
        // printFileLine(&localCnmInfo[channel]);
        // std::cout << std::endl;
        sem_post(semaphore1);
        sem_wait(semaphore2);
    // EXEC 
    } else if (nmcMode[channel] && pkt->getAddr() >= EXEC_START && pkt->getAddr() < EXEC_END) {
        hostAddrBase = pmemAddr_copy - RangeStart_copy + ADDR_OFFSET;
        localCnmInfo[channel].address = pkt->getAddr() - ADDR_OFFSET; 

        if(pkt->isRead()) {
#if AB_MODE
            bankParity = (uint64_t) (localCnmInfo[channel].address & MASK_BANK_PARITY);
            // std::cout << std::hex << "bankParity " << bankParity << std::endl;
            addrRemoveBABG = (uint64_t) ((localCnmInfo[channel].address & MASK_REMOVE_BABG) + (uint64_t) hostAddrBase);
            //addrRemoveBABG = (uint64_t)((uint64_t)hostAddrBase & MASK_REMOVE_BABG); // TODO why is this here, is it wrong or problematic?

            // std::cout << "Reading all banks addressed by " << std::hex << (uint64_t) hostAddrBase << std::endl;
            // std::cout << "Base address is " << addrRemoveBABG << std::endl;

            if (bankParity == 0) {    // Even bank
                for (int i = 0; i < NUM_BG; i++) {  // Loop over Bank Groups
                    for (int j = 0; j < NUM_BANK; j+=2) {   // Loop over even banks
                        memcpy(&localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+(j/2))], (uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK)), DWORDS_PER_COL*sizeof(uint64_t));
                        // std::cout << "pointed data = " << (uint64_t) *((uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK))) << std::endl;
                        // std::cout << std::showbase << std::hex << "Packet Info Even address = " << localCnmInfo[channel].address  << "\t" << localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+(j/2))] << std::endl; 
                    }
                }
            } else {    // Odd bank
                for (int i = 0; i < NUM_BG; i++) {  // Loop over Bank Groups
                    for (int j = 1; j < NUM_BANK; j+=2) {   // Loop over even banks
                        memcpy(&localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+((j-1)/2))], (uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK)), DWORDS_PER_COL*sizeof(uint64_t));
                        // std::cout << "pointed data = " << (uint64_t) *((uint8_t*)(addrRemoveBABG + (i << SHIFT_BG) + (j << SHIFT_BANK))) << std::endl;
                        // std::cout << std::showbase << std::hex << "Packet Info Even address = " << localCnmInfo[channel].address  << "\t" << localCnmInfo[channel].dataArray[DWORDS_PER_COL*(NUM_BANK/2*i+((j-1)/2))] << std::endl;
                    }
                }
            }
#else
            memcpy(localCnmInfo[channel].dataArray, hostAddrBase+localCnmInfo[channel].address, DWORDS_PER_COL*sizeof(uint64_t));
#endif
        }
        localCnmInfo[channel].RDcmd = pkt->isRead();
        localCnmInfo[channel].issuedTick = curTick();
        localCnmInfo[channel].simCycle = 0;

        copyFileLine(&sharedCnmInfo[channel], &localCnmInfo[channel]);
        // std::cout << "send to SystemC" << std::endl;
        // printFileLine(&localCnmInfo[channel]);
        // std::cout << std::endl;
        sem_post(semaphore1);
        sem_wait(semaphore2);
        if (pkt->isWrite()) {
            // std::cout << "Address " << pkt->getAddr() << "localCnmInfo.address" << localCnmInfo[channel].address << std::endl;
            channelCnmWrite.push_back(channel);
            schedule(advanceOneCycle_event, curTick() + 3333);
        }
    }
        
}

void NMCcores::copyhostAddr(uint8_t *hostAddrPart) {
    pmemAddr_copy = hostAddrPart;
    // std::cout << "Copied pmemAddr " << std::hex << std::showbase << (uint64_t) pmemAddr_copy << std::endl;
}

void NMCcores::copyRangeStart(uint64_t rngStrt) {
    RangeStart_copy = rngStrt;
}

void NMCcores::printSem() {
    std::cout << "sem1 " << semaphore1 << std::endl;
    std::cout << "sem2 " << semaphore2 << std::endl;
}

NMCcores*
NMCcoresParams::create()
{
    return new NMCcores(this);
}

void NMCcores::initSharedMemory() {
    *sharedLastCmd = 0;
    for (int i = 0; i < NUM_SIM_CHANNEL; i++) {
        nmcMode[i] = 0; // Initialize all channels at memory mode
        sharedCnmInfo[i].address = 0;
        for (int j = 0; j < DWORDS_PER_COL*CORES_PER_PCH; j++) {
            sharedCnmInfo[i].dataArray[j] = 0;
        }
        sharedCnmInfo[i].issuedTick = 0;
        sharedCnmInfo[i].simCycle = 0;
        sharedCnmInfo[i].nmcMode = 0;
        sharedCnmInfo[i].RDcmd = 1;
    }
}

void NMCcores::endSystemCSim() {
    std::cout << "NMCcores ExitCallback" << std::endl;

    *sharedLastCmd = 1;
    
    sem_post(NMCcores::semaphore1);
    sem_wait(NMCcores::semaphore2);
     
    sem_unlink(NMCcores::semName1.c_str());
    sem_unlink(NMCcores::semName2.c_str());
    munmap(NMCcores::sharedMemPtr, NUM_SIM_CHANNEL*sizeof(FileLine) + sizeof(uint8_t));
    shm_unlink(shmName.c_str());
    // kill(pid, SIGTERM);
    std::cout << "unmapped everything" << std::endl;
}

void
NMCcores::NMCcoresExitCallback::process()
{
    nmc->endSystemCSim();
}
