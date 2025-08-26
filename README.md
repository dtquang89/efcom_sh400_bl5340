# Zephyr Application

This repository contains a blinky application. The main purpose of this
repository is to serve as a reference on how to structure Zephyr-based
applications.

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Prerequisites

Install dependencies package: `CMake` `Python` `Devicetree compiler`

```shell
# For Windows
winget install Kitware.CMake Ninja-build.Ninja oss-winget.gperf python Git.Git oss-winget.dtc wget 7zip.7zip
```

`Python3` and `west` need to be installed on the system

```shell
# to check if python is installed
python --version
pip3 install -U west
# Get the information about west and where it is located
pip3 show -f west
## It is necesssary to add the path-to-west-scripts in to PATH environment variables on Windows OS.
## It should be added in System -> Advanced System Settings -> Environment Variables -> Edit..
# Check if west is successfully install on the system
west --version
```

It is necessary to add `west`, `cmake` and other installed packages to `PATH`, so the commands can be used by the system.
In Windows, it can be done following here: System -> Advanced System Settings -> Environment Variables -> Edit..

Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html) to install Zephyr SDK.

### Initialization

The first step is to initialize the workspace folder (``efcom_workspace``) where
the ``efcom_sh400_bl5340`` application and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize efcom-workspace for the application (main branch)
# depends on your setup, you would need to have a token in order to clone the repo
west init -m https://github.com/dtquang89/efcom_sh400_bl5340 --mr develop efcom_workspace
# update Zephyr modules
cd efcom_sh400_bl5340
west update
west zephyr-export
```

### Project structure - A top-level directory

    .
    ├── app                 # Source code of main application
    ├── boards              # Board definition
    ├── doc                 # Doxygen
    ├── drivers             # Sensor drivers
    ├── dts                 # Binding device-tree for sensors and other devices
    └── CMakeLists.txt      # Build file system
    └── KConfig             # Configuration for macros
    └── west.yml            # Manifest to specify repository of the modules.

### Building and running

To build the application, run the following command:

```shell
cd efcom_sh400_bl5340
west build -b $BOARD app
```

where `$BOARD` is the target board.

You can use the `sh400_bl5340/nrf5340/cpuapp` board found in this
repository. Note that Zephyr sample boards may be used if an
appropriate overlay is provided (see `app/boards`).

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD app -DCMAKE_BUILD_TYPE=DEVELOP
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```

### Documentation

A minimal documentation setup is provided for Doxygen and Sphinx. To build the
documentation first change to the ``doc`` folder:

```shell
cd doc
```

Before continuing, check if you have Doxygen installed. It is recommended to
use the same Doxygen version used in [CI](.github/workflows/docs.yml). To
install Sphinx, make sure you have a Python installation in place and run:

```shell
pip install -r requirements.txt
```

API documentation (Doxygen) can be built using the following command:

```shell
doxygen
```

The output will be stored in the ``_build_doxygen`` folder. Similarly, the
Sphinx documentation (HTML) can be built using the following command:

```shell
make html
```

The output will be stored in the ``_build_sphinx`` folder. You may check for
other output formats other than HTML by running ``make help``.
