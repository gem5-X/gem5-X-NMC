# Cross-Layer NMC Exporation Framework
 This repository contains the framework that enables the exploration of Near Memory Computing (NMC) designs from hardware to system level.


The initial release is described in the following paper:

>**Crossing the Layers and Dotting the Details:Systematic Exploration of Near-Memory Computing**


## Dependencies

- GCC compiler supporting C++17
- CMake >= 3.5
- SystemC 2.3 build with C++11.
- [Ramulator](https://github.com/CMU-SAFARI/ramulator) patched with the files in ramulator_files.
- [Eigen](ba4d7304e2e165a71187a5ff7b6799733e0025d4) 

## Execution


## Project structure

- 📁 [**ANEMOS**:](./ANEMOS/) Configurable NMC Architecture Template
    - 📁 [**build**:](./build/) build folder.
    - 📁 [**eda_script**:](./eda_scripts/) TCL files for:
        - 📁 [**catapult**:](./eda_scripts/catapult/) high level synthesis of SystemC model using Catapult HLS. 
        - 📁 [**genus**:](./eda_scripts/genus/) synthesis and power estimation of RTL model using Cadence Genus. 
        - 📁 [**questasim**:](./eda_scripts/questasim/) simulation of SystemC and RTL models. 
    - 📁 [**inputs**:](./inputs/) files and traces employed for the programming interface.
    - 📁 [**ramulator_files**:](./ramulator_files/) patched files to support all-bank DRAM mode.
    - 📁 [**scripts**:](./scripts/) bash scripts to run parameterized explorations.
    - 📁 [**src**:](./src/) source files of SystemC template.
    - 📁 [**waveforms**:](./waveforms/) GTKWave format files to analyze VCD waveforms.
- 📁 [**gem5-x-nmc**:](./gem5-x-nmc/) Full-System Simulator: Extended Version of [gem5X](https://github.com/gem5-X/gem5-X) to support NMC exploration
- 📁 [**Software Stack**:](./softwareStack/) Configurable NMC Architecture Template

