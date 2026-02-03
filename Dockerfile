FROM ubuntu:16.04

# Define default shell
SHELL ["/bin/bash", "-c"] 

# Environment variables
ENV GEM5XNMC_FW=/gem5-X-NMC
ENV ANEMOS=$GEM5XNMC_FW/ANEMOS
ENV SOFTWARELIB=$GEM5XNMC_FW/softwareStack
ENV GEM5_X_NMC=$GEM5XNMC_FW/gem5-x-nmc
ENV RAMULATOR_DIR=$GEM5XNMC_FW/ramulator  
ENV SYSTEMC_HOME=$GEM5XNMC_FW/systemc 
ENV CPP_APPS=$SOFTWARELIB/Applications

# Optimize the mirrors for a fast APT image
RUN sed -i 's/htt[p|ps]:\/\/archive.ubuntu.com\/ubuntu\//mirror:\/\/mirrors.ubuntu.com\/mirrors.txt/g' /etc/apt/sources.list

# Enable cache for APT and PIP for faster docker image creation time
ENV PIP_CACHE_DIR=/var/cache/buildkit/pip
RUN mkdir -p $PIP_CACHE_DIR
RUN rm -f /etc/apt/apt.conf.d/docker-clean

# Install the APT packages
RUN --mount=type=cache,target=/var/cache/apt \
	apt-get update && \
	apt-get install -y build-essential \
        device-tree-compiler cmake wget git vim g++-aarch64-linux-gnu \
	python python-dev python-six gcc g++ software-properties-common gcc-arm-linux-gnueabihf gcc-aarch64-linux-gnu zlib1g-dev m4 libprotobuf-dev libgoogle-perftools-dev libprotoc-dev protobuf-compiler screen gdb \
	&& add-apt-repository universe \
	&& apt-get update \
	&& apt-get -y install python-pip libprotobuf-c1 libprotobuf-c-dev python-protobuf scons swig protobuf-c-compiler libboost-all-dev diod libxerces-c-dev automake autoconf perl flex bison byacc libhdf5-dev libelf-dev cmake-curses-gui \
	&& pip install protobuf \
	&& rm -rf /var/lib/apt/lists/*

# Copy the project files to the container
COPY . /gem5-X-NMC

# Build the m5 library
# RUN make -C cnm-framework/pim-cores/cnmlib/util/m5 --file=Makefile.aarch64
WORKDIR /gem5-X-NMC/gem5-x-nmc/util/m5
RUN make --file=Makefile.aarch64

RUN chmod +x /gem5-X-NMC/scripts/install_systemC.sh
RUN /gem5-X-NMC/scripts/install_systemC.sh

ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${SYSTEMC_HOME}/lib"
# ENV SYSTEMC_HOME="${SYSTEMC_HOME}:${SYSTEMC_HOME}/include"

RUN chmod +x /gem5-X-NMC/scripts/install_ramulator.sh
RUN /gem5-X-NMC/scripts/install_ramulator.sh

# RUN chmod +x /gem5-X-NMC/scripts/build_gem5.sh
# RUN /gem5-X-NMC/scripts/build_gem5.sh

# Set working directory
WORKDIR /gem5-X-NMC