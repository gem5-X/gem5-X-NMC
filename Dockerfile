FROM ubuntu:16.04

ARG UNAME=kodra
ARG USER_ID=255285
ARG GROUP_ID=10433


# Define default shell
SHELL ["/bin/bash", "-c"] 

# Environment variables
ENV SYSTEMC_HOME=$SYSTEMC_HOME:/home/kodra/shares/local/scrap/include/
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/kodra/shares/local/scrap/lib/
RUN ldconfig
# echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/kodra/shares/local/scrap/lib/' >>$SINGULARITY_ENVIRONMENT
# echo 'export SYSTEMC_HOME=$SYSTEMC_HOME:/home/kodra/shares/local/scrap/include/' >>$SINGULARITY_ENVIRONMENT

# Optimize the mirrors for a fast APT image
RUN sed -i 's/htt[p|ps]:\/\/archive.ubuntu.com\/ubuntu\//mirror:\/\/mirrors.ubuntu.com\/mirrors.txt/g' /etc/apt/sources.list

# Enable cache for APT and PIP for faster docker image creation time
ENV PIP_CACHE_DIR=/var/cache/buildkit/pip
RUN mkdir -p $PIP_CACHE_DIR
RUN rm -f /etc/apt/apt.conf.d/docker-clean

# Install the APT packages
RUN --mount=type=cache,target=/var/cache/apt \
	apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
        vim python python-dev python-six gcc g++ software-properties-common \
        gcc-arm-linux-gnueabihf device-tree-compiler zlib1g-dev m4 \
        libprotobuf-dev libgoogle-perftools-dev libprotoc-dev protobuf-compiler \
        screen gdb python-pip libprotobuf-c1 libprotobuf-c-dev python-protobuf \
        scons swig protobuf-c-compiler libboost-all-dev diod libxerces-c-dev \
        automake autoconf perl flex bison byacc libhdf5-dev libelf-dev \
        cmake-curses-gui

# Add universe repo
RUN add-apt-repository universe && \
    apt-get update

# Python
RUN pip install protobuf		

# Fix asm include symlink (only if needed)
RUN ln -s /usr/include/asm-generic /usr/include/asm || true

# # Copy the project files to the container
# COPY . /CrossLayerNMC

# Create the working directory where the host directory will be mounted
# RUN mkdir -p /CrossLayerNMC
# RUN chmod +0777 /CrossLayerNMC 

# Environment variables as in Singularity
ENV CROSSLAYER_FW="/CrossLayerNMC"
ENV SYSTEMC_HOME="$CROSSLAYER_FW/systemc" 
ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SYSTEMC_HOME/lib/"


# @TODO Build m5 in the scripts (once initially)
# # Build the m5 library
# # RUN make -C cnm-framework/pim-cores/cnmlib/util/m5 --file=Makefile.aarch64
# WORKDIR /CrossLayerNMC/gem5-x-nmc/util/m5
# RUN make --file=Makefile.aarch64

RUN groupadd -g $GROUP_ID -o $UNAME
RUN useradd -m -u $USER_ID -g $GROUP_ID -o -s /bin/bash $UNAME
USER $UNAME


# Set working directory
WORKDIR /home/$UNAME
