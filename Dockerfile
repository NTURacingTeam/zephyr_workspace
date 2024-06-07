FROM zephyrprojectrtos/zephyr-build:latest

# switch to root user for elevated permission
USER root

RUN apt-get update && apt-get install -y --no-install-recommends \
    htop \
    openocd \
    tmux \
    vim \
    && rm -rf /var/lib/apt/lists/*

# fix for "bash: /home/user/.bash_completion.d/*: No such file or directory"
RUN rm -rf /home/user/.bash_completion /home/user/.bash_completion.d

# change default shell to bash
RUN chsh -s /bin/bash user

USER user
ENV USER user
