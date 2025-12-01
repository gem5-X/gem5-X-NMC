FROM ubuntu:16.04

# Define default shell
SHELL ["/bin/bash", "-c"] 

# Environment variables
ENV CROSSLAYER_FW=/CrossLayerNMC
ENV ANEMOS=$CROSSLAYER_FW/ANEMOS
ENV SOFTWARELIB=$CROSSLAYER_FW/softwareStack
ENV GEM5_X_NMC=$CROSSLAYER_FW/gem5-x-nmc
ENV RAMULATOR_DIR=$CROSSLAYER_FW/ramulator  
ENV SYSTEMC_HOME=$CROSSLAYER_FW/systemc 

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
	&& rm -rf /var/lib/apt/lists/*

# Copy the project files to the container
COPY . /CrossLayerNMC

# Build the m5 library
# RUN make -C cnm-framework/pim-cores/cnmlib/util/m5 --file=Makefile.aarch64
WORKDIR /CrossLayerNMC/gem5-x-nmc/util/m5
RUN make --file=Makefile.aarch64

RUN chmod +x /CrossLayerNMC/scripts/install_systemC.sh
RUN /CrossLayerNMC/scripts/install_systemC.sh

RUN chmod +x /CrossLayerNMC/scripts/install_ramulator.sh
RUN /CrossLayerNMC/scripts/install_ramulator.sh

# Set working directory
WORKDIR /CrossLayerNMC