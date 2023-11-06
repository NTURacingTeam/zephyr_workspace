# Zephyrproject

## Usage

### Docker Image Install

First, build `zephyr` docker image:

```bash
docker build . -t zephyr
```

Second, run `zephyr` docker container with the built image:

```bash
docker compose up -d
```

Third, attach to the shell of `zephyr` docker container:

```bash
docker exec -it zephyr bash
```

### West Workspace Initialization

In `/workdir` of the container, initialize it as west workspace:

```bash
west init -l applications
west update
```

### VS Code Support

Attach vs code to the container, then open workspace form `/workdir/zephyr.code-workspace` in vs code.

This repository provides plenty utilities tasks in [.vscode/tasks.json](.vscode/tasks.json) and launches in [.vscode/launch.json](.vscode/launch.json) to build, flash and debug zephyr.

## Reference
project structured as: [zephyrproject-rtos/example-application](https://github.com/zephyrproject-rtos/example-application)
vs code workspace config modified from: [KozhinovAlexander/zephyr_vscode_workspace](https://github.com/KozhinovAlexander/zephyr_vscode_workspace)
