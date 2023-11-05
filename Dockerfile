FROM zephyrprojectrtos/zephyr-build:latest

# switch to root user for perssion
USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    openocd \
    tmux \
    vim \
    && rm -rf /var/lib/apt/lists/*

# micro ros dependencies
RUN pip3 install --no-cache-dir \
    catkin_pkg \
    lark-parser \
    empy \
    colcon-common-extensions

# fix for "bash: /home/user/.bash_completion.d/*: No such file or directory"
RUN rm -rf /home/user/.bash_completion /home/user/.bash_completion.d

# change default shell to bash
RUN chsh -s /bin/bash user

USER user
ENV USER user
