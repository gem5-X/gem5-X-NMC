/* 
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 * 
 * Intrinsics to perform read and write operations triggering CNM DRAM execution.
 *
 */

#ifndef CNM_INTRINSICS_H
#define CNM_INTRINSICS_H

#ifdef CHECKER
#include <iostream>

void strData(uint64_t *addr, uint64_t data) {
    std::cout << "Executing command ";
    std::cout << std::showbase << std::hex << addr << " WR " << data << std::endl;
}

void ldrData(uint64_t *addr, uint64_t* data) {
    std::cout << "Executing command ";
    std::cout << std::showbase << std::hex << addr << " RD " << *data << std::endl;
}

#else

// Assembly intrinsics to store 64 bits of data in a memory address
__attribute__((always_inline)) void strData(uint64_t *addr, uint64_t data){
        __asm__ volatile(
                "STR %[register1], [%[register2],  0]\n\t"\
                : [register1] "+r" (data)
                : [register2] "r" (addr)
                :
                );			
}

// Assembly intrinsics to load 64 bits of data from a memory address
__attribute__((always_inline)) void ldrData(uint64_t *addr, uint64_t* data){
        __asm__ volatile(
                "LDR %[register1], [%[register2],  0]\n\t"\
                : [register1] "+r" (*data)
                : [register2] "r" (addr)
                :
                );		
}

#endif  // CHECKER

#endif  // CNM_INTRINSICS_H