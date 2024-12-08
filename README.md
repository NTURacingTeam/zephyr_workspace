# Zephyr Workspace

Workspace for hosting Zephyr RTOS applications targeting VS Code as the IDE.

## Requirements

- Linux operating system (native or WSL)
- Docker
- VS Code

## Usage

### Clone Repository

Clone this repository and change directory to it:

```bash
git clone https://github.com/NTURacingTeam/zephyr_workspace.git
cd zephyr_workspace
```

### Docker Image Install

Build and run `zephyr` docker container using `docker compose`:

```bash
docker compose up -d
```

Then, attach to the shell of `zephyr` docker container:

```bash
docker exec -it zephyr bash
```

### Zephyr Workspace Initialization

In `/workdir` of the container, initialize it as zephyr workspace:

```bash
west init -l <application_name>
west update
```

where `<application_name>` is the name of the application you want to develop, or you may use `blink` as a reference application included in this repository.

### Switching to Other Applications

To switch to other applications with different `west` configuration, first remove the current west config and reinitialize it:

```bash
# in /workdir
rm -rf .west
west init -l <application_name>
west update
```

### VS Code Support

Attach vs code to the container, then open workspace form `/workdir/zephyr.code-workspace` in VS Code.

#### VS Code Tasks and Launch

This repository provides plenty utilities tasks in [.vscode/tasks.json](.vscode/tasks.json) and launch files in [.vscode/launch.json](.vscode/launch.json) to build, flash and debug zephyr.

#### Settings

According to your application, you may need to modify the following settings in `/workdir/zephyr.code-workspace` in order to make the tasks and launch files work properly:

- "folders" -> "path": the path to your application
- "settings" -> "bsp" -> "board": the board you are using
- "settings" -> "bsp" -> "board_root": the path to the board
- "settings" -> "bsp" -> "soc": the soc you are using
- "settings" -> "bsp" -> "vendor": the vendor of the board

> Note: VS Code caches the settings, so you may need to restart VS Code to make the changes take effect.

## References

- Project structured as: [zephyrproject-rtos/example-application](https://github.com/zephyrproject-rtos/example-application)
- VS code workspace config modified from: [KozhinovAlexander/zephyr_vscode_workspace](https://github.com/KozhinovAlexander/zephyr_vscode_workspace)
