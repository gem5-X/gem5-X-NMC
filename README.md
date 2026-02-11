# Gem5-X-NMC Exporation Framework
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
    - 📁 [**Applications**:](./softwareStack/Applications) Folder for storing compiled full CNN applications.
    - 📁 [**CNNs**:](./softwareStack/CNNs) CNN C++ models definitions
    - 📁 [**eigen**:](./softwareStack/eigen) Open-source library for optimized C++ arithmetic and data structures
    - 📁 [**NMClib**:](./softwareStack/NMClib) Library for mapping kernels and generating the DRAM commands for NMC execution.
    - 📁 [**tinytensorlib**:](./softwareStack/tinytensorlib) Extended library from [TinyTensorLib](https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=10622095), supporting CPU and NMC execution of full inferences.

- 📄 Dockerfile: Container used for compiling NMC cores executable instances, applications to be executed on gem5-x-nmc and gem5-x-nmc simulation.

## Flow

### 1. Setting up the framework
Firstly, full system images are required for being able to launch full system simulation. The ones used by us were obtained through the official [gem5X website](https://github.com/gem5-X/gem5-X).

Once you obtain them, for consistency with the rest of this repository, please store the obtained disks and binaries in a folder named **"full_system_images"**, directly inside the root folder, i.e. gem5-X-NMC.

To set up the framework, set the paths of the folders, build the docker image and install the needed libraries, run from the root folder of the repository:

```
source scripts/setUpFramework.sh <name_of_docker_image>
```
❗❗❗ **Make sure to rebuild the docker image after each time new files are generated throughout the following steps.** ❗❗❗

### 2. Generating NMC Architecture Models (executables)
Use the docker image to build the executables of the NMC Architecture models. In the [definition file of ANEMOS, defs.h](./ANEMOS/src/defs.h) you can change the parameters of the design such as: type of DRAM or number of entries in the registers. For generating the NMC cores executables for **full system simulation**, set in [defs.h](./ANEMOS/src/defs.h) GEM5  1. 

To generate the executables for each DRAM type, and for the default values of the other architecture parameters in [defs.h](./ANEMOS/src/defs.h), you can run the commnad: 
```
docker run --name <container_name> \
-it --rm \
-v /full_host_path/gem5-X-NMC/gem5-x-nmc/ext/NMCcores/:/gem5-X-NMC/gem5-x-nmc/ext/NMCcores/ \
<image_name> \
bash -c "sh scripts/build_NMC_cores.sh"
```
For your own customized designs you can follow the same flow as shown in the script, but ensure that the changes in the [defs.h](./ANEMOS/src/defs.h) file are reflected in the docker image being used.

<!-- ❗❗❗ **Make sure to rebuild the docker image to include NMC-cores executable files for each DRAM type in the container.** ❗❗❗ -->

### 3. Building gem5-x-nmc (FS simulator interfaced with NMC models)

For building a gem5-x-nmc instance, run the following command inside the (updated) docker container.

```
docker run --name <container_name> \
    -it --rm \
    -v /full_host_path/gem5-X-NMC/gem5-x-nmc/build/ARM/:/gem5-X-NMC/gem5-x-nmc/build/ARM \
    <image_name> \
    bash -c "sh scripts/build_gem5.sh <nmc_executable_path> <gem5_build_name>"
```
**Example**:
```
docker run --name <container_name> \
    -it --rm \
    -v /full_host_path/gem5-X-NMC/gem5-x-nmc/build/ARM/:/gem5-X-NMC/gem5-x-nmc/build/ARM \
    <image_name> \
    bash -c "sh scripts/build_gem5.sh /gem5-X-NMC/gem5-x-nmc/ext/NMCcores/nmc-cores-HBM HBM"
```
<!-- ❗❗❗ **Make sure to rebuild the docker image to include the gem5-x-nmc builds for each DRAM type in the container.** ❗❗❗ -->

### 4. Building applications 
We provide 4 CNN models, and to compile them use the following command.
```
docker run --name <container_name> \
-it --rm \
-v /full_host_path/gem5-X-NMC/softwareStack/Applications:/gem5-X-NMC/softwareStack/Applications \
<image_name> \
bash -c "cd /gem5-X-NMC/softwareStack/CNNs && make <target> SUFFIX=<optional>"
```
You can create new models by providing the network definitions as header files, and using the functions in [tinytensorlib](./softwareStack/tinytensorlib) to implement the layers for both CPU and NMC execution. Update the Makefile to include the new targets. 

<!-- ❗❗❗ **Make sure to rebuild the docker image to include the CNN C++ applications in the container.** ❗❗❗ -->

### 5. Running simulations with the generated applications
For running simulations using the full-system simulator gem5-x-nmc, you need two active terminals. The first terminal is used for launching the gem5-x-nmc simulation in an interactive mode, from where we will retrieve the port where we need to connect for accessing the simulated full-system. The second terminal is used for connecting to the terminal. 

In the first terminal run the container, that will be used for the launch of the gem5-x-nmc simulation using the command:

```
docker run -it --rm --name <container_name> \
-v /full_host_path/gem5-X-NMC/full_system_images:/gem5-X-NMC/full_system_images \
<image_name>
```

In the first terminal, after launching gem5-x-nmc simulation (explained below) you will see this message: "system.terminal: Listening for connections on port <port_number>".

In the second terminal run the following command to connect to the port, using as container name the same as the one above:
```
docker exec -it <container_name> bash -c "cd /gem5-X-NMC/gem5-x-nmc/util/term && ./m5term 127.0.0.1 <port_number>"
```

#### First Checkpoint
This step needs to be done **ONLY ONCE** when using our provided scripts and flow. 

*If you use a customized design, ensure that any modifications to the gem5-x system configuration do not break checkpoint restoration.* 

In the first terminal where the docker container is running, execute the following commad to boot the full-system: 

```
sh gem5-x-cnm/sripts/launch/first_checkpoint_launch.sh
```

During the full-system (FS) boot in gem5-x-nmc, a checkpoint is automatically created once the OS finishes booting and the m5 terminal becomes available in the second terminal. This initial boot runs on the AtomicSimpleCPU, so no timing information is collected yet. This checkpoint will be used for future simulations where you can switch to a more accurate in-order CPU model (MinorCPU) for timing simulations.

If the simulation is still running after the boot in the second terminal, where we are connected to the port of the gem5-x-nmc, you can safely exit it from the terminal by using:

```
m5 exit
```

In the folder /gem5-X-NMC/gem5-x-nmc/gem5_outputs/first_checkpoint in the container you will see the checkpoint as a  folder named "cpt.number", where number is the tick number where the checkpoint was done.  


❗❗❗ **Including the checkpoint in future simulations.** ❗❗❗

- ❗ Make sure to copy the checkpoint folder on the host WITHOUT changing its name and rebuild the docker image to include it in the future simulations containers.
- ❗ Apply the patch below to pass the path of the checkpoint to the launching scripts.
```
sh pass_checkpoint_path.sh <container_path_to_checkpoint>
```
- ❗ Rebuild the docker image.

#### General simulation

After having rebuilt the docker image, run the docker in the first terminal as explained above and there run the following script: 

```
sh gem5-x-nmc/scripts/launch/docker_launch.sh <simulation_name>
```

In the second terminal, after connecting to the port as shown above, firstly mount the shared folder where the Applications are located and then enter the folder using the following commands:

```
sh mount.sh /gem5-X-NMC/softwareStack/Applications
cd /mnt
```

In that terminal you can see and execute the applications generated via the softwareStack support.
```
ls
./<name_of_application>
``` 
