# docker image for zephyr 3.7 LTS
FROM ghcr.io/zephyrproject-rtos/zephyr-build:v0.26-branch

# switch to root user for elevated permission
USER root

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    	htop \
    	minicom \
    	openocd \
    	tmux \
    	vim && \
    rm -rf /var/lib/apt/lists/*

# change default shell to bash
RUN chsh -s /bin/bash user

USER user
ENV USER user
