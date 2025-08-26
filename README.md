# Zephyr Workspace

Workspace for developing [Zephyr RTOS](https://www.zephyrproject.org) applications targeting VS Code as the IDE.

## Features

- Easy Zephyr installation via Docker
- VS Code Task configurations for:
    - Building
    - Cleaning
    - Flashing
    - Testing
- VS Code Launch files for:
    - Debugging remotely on target board
    - Debugging natively on host machine

## Usage Tutorial

### 0. Requirements

- [VS Code](https://code.visualstudio.com/download)
- Linux environment (supports both native install and [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)), in which the following dependencies are installed;
    - [Docker Engine](https://docs.docker.com/engine/install)
    - [stlink-tools](https://github.com/stlink-org/stlink?tab=readme-ov-file#installation), which can be installed by

        ```bash
        sudo apt install stlink-tools
        ```

### 1. Clone Repository

Open a Linux terminal, then clone this repository and change directory to it:

```bash
git clone https://github.com/NTURacingTeam/zephyr_workspace.git
cd zephyr_workspace
```

### 2. Build and Run Docker Container

Build and run a Docker container named `zephyr` using Docker Compose:

```bash
docker compose up -d
```

Then, attach to the shell of it:

```bash
docker exec -it zephyr bash
```

### 3. Initialize Zephyr Workspace

In the `/workdir` directory of the container, initialize it as Zephyr workspace:

```bash
west init -l <application_name>
west update
```

where `<application_name>` is the name of the application you want to develop. You may use `blink` as a reference application included in this repository.

> [!NOTE]
> Since every Zephyr application has its own [west manifest](https://docs.zephyrproject.org/latest/develop/west/manifest.html), when switching to another application, you need to remove the current west configuration and reinitialize it:
> 
> ```bash
> # in /workdir
> rm -rf .west
> west init -l <application_name>
> west update
> ```

### 4. Attach VS Code to WSL2 (required only for WSL2)

Install the [Remote - WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl) VS Code Extension and bring up the Command Palette with `Ctrl+Shift+P` or `F1`. Then we can attach to WSL2 by selecting `WSL: Connect to WSL`.

After this step, in the lower left corner of the Status Bar, you should see that you're connected to your **WSL: Ubuntu**.

![](https://code.visualstudio.com/assets/docs/remote/wsl-tutorial/wsl-status-bar.png)

### 5. Attach VS Code to Container

First install both [Docker](https://marketplace.visualstudio.com/items?itemName=ms-azuretools.vscode-docker) and [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) VS Code Extensions. Second, bring up the Command Palette with `Ctrl+Shift+P` or `F1` and select `Dev Containers: Attach to Running Container...`. Finally, select the `zephyr` container from the list to attach to VS Code to the container we just created.

After this step, in the lower left corner of the Status Bar, you should see that you're connected to your **Container zephyr (zephyr)**, similar to the following:

![](https://learn.microsoft.com/zh-tw/training/modules/use-docker-container-dev-env-vs-code/media/remote-indicator.png)

> [!NOTE]
> The VS Code Extensions are not shared across the host machine, WSL2 and Docker containers, so you must install Docker and Dev Containers extension in the WSL2 VS Code if you are using WSL2.

Then open VS Code Workspace using `File -> Open Workspace from File` from the menu bar and select `/workdir/zephyr.code-workspace` (just replace the default `/home/user` with this).

### 5. Configure for the Application and Target Board

Before we can build and flash the application to target board, have to edit the `zephyr.code-workspace` file to match the application and target board we are developing. Change the following settings:

- "folders" -> "path": the path to your application (default to `blink` reference application)
- "settings" -> "bsp": board support package settings
    - "board": the board you are using
    - "board_root": the path to the board
    - "soc": the soc you are using
    - "vendor": the vendor of the board
    
    (the default values for `NUCLEO-G474RE` are suffice for now)

> [!NOTE]
> Since VS Code caches settings, you may need to restart VS Code to make the changes take effect after editing settings in `zephyr.code-workspace` using `Developer: Reload Window` in Command Palette.

### 6. Build and Flash Application

First attach target board to your computer.

> [!NOTE]
> If you are using WSL2, by default USB devices are not accessible from WSL2. You need to follow [this guide](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) first and pass the attached board to WSL2 using `usbpid` command.

Then you can build and flash the application using the provided Tasks by pressing `Ctrl+Shift+P` or `F1` to open the Command Palette and selecting `Tasks: Run Task` and then `West Build and Flash`.

### 7. Debug Application

You can debug the application using the provided Launch files by pressing `F5` or selecting `Run and Debug -> Start Debugging` from the sidebar.

## References

- Project structured as: [zephyrproject-rtos/example-application](https://github.com/zephyrproject-rtos/example-application)
- VS code workspace config modified from: [KozhinovAlexander/zephyr_vscode_workspace](https://github.com/KozhinovAlexander/zephyr_vscode_workspace)
