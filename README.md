# Zephyr Application

This repository contains a blinky application. The main purpose of this
repository is to serve as a reference on how to structure Zephyr-based
applications.

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (``efcom_workspace``) where
the ``efcom_sh400_bl5340`` application and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize efcom-workspace for the application (main branch)
# depends on your setup, you would need to have a token in order to clone the repo
west init -m https://github.com/dtquang89/efcom_sh400_bl5340 --mr main efcom_workspace
# update Zephyr modules
cd efcom_sh400_bl5340
west update
west zephyr-export
```

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
west build -b $BOARD app -- -DEXTRA_CONF_FILE=debug.conf
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
