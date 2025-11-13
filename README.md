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

## Execution


## Project structure

- 📁 [**ANEMOS**:](./ANEMOS/) Configurable NMC Architecture Template: This is an extended version of the open-source repository [ANEMOS](https://github.com/gem5-X/ANEMOS/tree/main) to support Full-System (FS) Simuation.
- 📁 [**gem5-x-nmc**:](./gem5-x-nmc/) Full-System Simulator: Extended Version of [gem5X](https://github.com/gem5-X/gem5-X) to support NMC exploration
- 📁 [**Software Stack**:](./softwareStack/) Configurable NMC Architecture Template

