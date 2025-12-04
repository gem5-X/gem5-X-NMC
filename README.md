# Cross-Layer NMC Exporation Framework
 This repository contains the framework that enables the exploration of Near Memory Computing (NMC) designs from hardware to system level.


The purpose of the framework is described in the following paper:

>**Crossing the Layers and Dotting the Details:Systematic Exploration of Near-Memory Computing**


## Dependencies

- GCC compiler supporting C++11
- CMake >= 3.5
- SystemC 2.3 build with C++11.
- [Ramulator](https://github.com/CMU-SAFARI/ramulator) patched with the files in ramulator_files.
- [Eigen](https://gitlab.com/libeigen/eigen/-/tree/ba4d7304e2e165a71187a5ff7b6799733e0025d4) 

## Project structure

- 📁 [**ANEMOS**:](./ANEMOS/) Configurable NMC Architecture Template: This is an extended version of the open-source repository [ANEMOS](https://github.com/gem5-X/ANEMOS/tree/main) to **support Full-System (FS) Simuation**.\
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

- 📁 [**gem5-x-nmc**:](./gem5-x-nmc/) Full-System Simulator: Extended Version of [gem5X](https://github.com/gem5-X/gem5-X) to support NMC exploration from a system level. This approach accounts for the effects other system components on the scheduling of DRAM commands during simulation. 

- 📁 [**ramulator_files**:](./ramulator_files/) Patched files to support all-bank DRAM mode and FS simulation in gem5-x.

- 📁 [**scripts**:](./scripts/) Bash scripts to set up and utilize the framework for NMC explorations. 

- 📁 [**Software Stack**:](./softwareStack/) Software support for generating CNN applications, deployable on NMC architectures, integrated on a FS simulator.
    - 📁 [**Applications**:](./softwareStack/Applications): Folder for storing compiled full CNN applications.
    - 📁 [**CNNs**:](./softwareStack/CNNs): CNN C++ models definitions
    - 📁 [**eigen**:](./softwareStack/eigen): Open-source library for optimized C++ arithmetic and data structures
    - 📁 [**NMClib**:](./softwareStack/NMClib): Library for mapping kernels and generating the DRAM commands for NMC execution.
    - 📁 [**tinytensorlib**:](./softwareStack/tinytensorlib): Extended library from [TinyTensorLib](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=10622095), supporting CPU and NMC execution of full inferences.

- 📄 Dockerfile: Container used for compiling NMC cores executable instances, applications to be executed on gem5-x-nmc and gem5-x-nmc simulation.

## Flow

### 1. Setting up the framework
Firstly, full system images are required for being able to launch full system simulation. The ones used by us were obtained through the official [gem5X website](https://github.com/gem5-X/gem5-X).

Once you obtain them, for consistency with the rest of this repository, please store the obtained disks and binaries in a folder named **"full_system_images"**x, directly inside the root folder, i.e. CrossLayerNMC.

To set up the framework, set the paths of the folders, build the docker image and install the needed libraries, run from the root folder of the repository:

```
source scripts/setUpFramework.sh <name_of_docker_image>
```

### 2. Generating NMC Architecture Models (executables)
The generated docker image can now be used to build the executables of the NMC Architecture models. In the [definition file of ANEMOS, defs.h](./ANEMOS/src/defs.h) you can change the parameters of the design such as: type of DRAM or number of entries in the registers. For generating the NMC cores executable for **full system simulation**, set in [defs.h](./ANEMOS/src/defs.h) GEM5  1. 

To generate the executables for each DRAM type, and the other architecture parameters set using the default values in [defs.h](./ANEMOS/src/defs.h), you can run the commnad: 
```
docker run --name <container_name> -it <image_name> bash -c "sh scripts/build_NMC_cores.sh"
```
For your own customized designs you can follow the same flow as shown in the script, but ensure that the changes in the [defs.h](./ANEMOS/src/defs.h) file are reflected in the docker image being used.

After building the NMC cores executable instances ensure to copy/move them under the [NMCcores](./gem5-x-nmc/ext/NMCcores/) folder in gem5-x-nmc. In this step either update the docker image so that these files are always present in the container, or mount/bind them every time you use the initial image.

### 3. Building gem5-x-nmc with NMC integrated

For building a gem5-x-nmc instance, after copying the executables generated above in the correct folder run the following command inside the docker container.

```
docker run --name <container_name> -it <image_name> bash -c "sh scripts/build_gem5.sh <nmc_executable_path> <gem5_build_name>"
```
Example:
```
docker run --name <container_name> -it <image_name> bash -c "sh scripts/build_gem5.sh /CrossLayerNMC/gem5-x-nmc/ext/NMCcores/nmc-cores-HBM HBM"
```

### 4. Building applications 

### 5. Running simulations with the generated applications


