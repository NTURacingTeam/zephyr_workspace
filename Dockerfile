# docker image for zephyr 3.7 LTS
FROM ghcr.io/zephyrproject-rtos/zephyr-build:v0.26-branch

ARG DOXYGEN_VERSION=1.12.0
ENV DOXYGEN_VERSION=$DOXYGEN_VERSION

# switch to root user for elevated permission
USER root

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
	graphviz \
    	htop \
    	minicom \
    	openocd \
    	tmux \
    	vim && \
    rm -rf /var/lib/apt/lists/*

# install doxygen, overrides the default version in the base image
RUN wget ${WGET_ARGS} https://downloads.sourceforge.net/project/doxygen/rel-${DOXYGEN_VERSION}/doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz && \
	tar xf doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz -C /opt && \
	ln -fs /opt/doxygen-${DOXYGEN_VERSION}/bin/doxygen /usr/local/bin && \
	rm doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz

RUN pip3 install --no-cache-dir \
		breathe \
		sphinx \
		sphinx-rtd-theme

# change default shell to bash
RUN chsh -s /bin/bash user

USER user
