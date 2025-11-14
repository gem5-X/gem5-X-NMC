# Cross-Layer NMC Exporation Framework
 This repository contains the framework that enables the exploration of Near Memory Computing (NMC) designs from hardware to system level.


The purpose of the framework is described in the following paper:

>**Crossing the Layers and Dotting the Details:Systematic Exploration of Near-Memory Computing**


## Dependencies

- GCC compiler supporting C++17
- CMake >= 3.5
- SystemC 2.3 build with C++11.
- [Ramulator](https://github.com/CMU-SAFARI/ramulator) patched with the files in ramulator_files.
- [Eigen](https://gitlab.com/libeigen/eigen/-/tree/ba4d7304e2e165a71187a5ff7b6799733e0025d4) 

## Project structure

- 📁 [**ANEMOS**:](./ANEMOS/) Configurable NMC Architecture Template: This is an extended version of the open-source repository [ANEMOS](https://github.com/gem5-X/ANEMOS/tree/main) to support Full-System (FS) Simuation.\
For performing Area, Energy and NMC-only cycle-accurate simulations follow the original repository instructions on the above link. 
            
    Our Additions/Changes:
    - Updated subdir.mk files in [build](./ANEMOS/build) folder
    - New/Updated src and testbench files to interface with FS: 
        <!-- - src/cnm_device.cpp
        - src/cnm_device.h
        - src/defs.h  updated with MACRO GEM5: set to 1 for FS 
        - src/tb/cnm_driver_gem5.cpp 
        - src/tb/cnm_driver.cpp 
        - src/tb/cnm_driver.h
        - src/tb/cnm_main.cpp
        - src/tb/cnm_main.h
        - src/tb/cnm_monitor.cpp
        - src/tb/cnm_monitor.h -->
    - Updated [ANEMOS ramulator_files](./ANEMOS/ramulator_files/) to match with the ones used in FS simulation [gem5-x-nmc ramulator files](./gem5-x-nmc/ext/ramulator/)

- 📁 [**gem5-x-nmc**:](./gem5-x-nmc/) Full-System Simulator: Extended Version of [gem5X](https://github.com/gem5-X/gem5-X) to support NMC exploration

- 📁 [**ramulator_files**:](./ramulator_files/) Patched files to support all-bank DRAM mode and FS simulation in gem5-x.

- 📁 [**scripts**:](./scripts/) Bash scripts to set up and utilize the framework for NMC explorations. 

- 📁 [**Software Stack**:](./softwareStack/) Software support for generating CNN applications, deployable on NMC architectures, integrated on a FS simulator.
    - 📁 [**CNNs**:](./softwareStack/CNNs): CNN C++ models definitions
    - 📁 [**eigen**:](./softwareStack/eigen): Open-source library for optimized C++ arithmetic and data structures
    - 📁 [**NMClib**:](./softwareStack/NMClib): Library for mapping kernels and generating the DRAM commands for NMC execution.
    - 📁 [**tinytensorlib**:](./softwareStack/tinytensorlib): Extended library from [TinyTensorLib](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=10622095), supporting CPU and NMC execution of full inferences.

- 📄 Dockerfile: Container used for compiling applications, to be executed on gem5-x-nmc.

- 📄 Singularity Image: Container used for building and running NMC models and gem5-x-nmc.

## Execution



