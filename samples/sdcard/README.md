# Demonstration application
Firmware application to write and read to/from SD Card.

## Prerequisites

Format the SD card under FAT format.

On MacOs, open Terminal, run the following commands to perform format the SD Card

```shell
# List all connected drives
diskutil list
```

Identify the identifier for your MicroSD card (e.g., `disk6`). 

```shell
# Suppose the SD card identifier is /dev/disk6
diskutil unmountDisk /dev/disk6
sudo diskutil eraseDisk FAT32 SDCARD MBRFormat /dev/disk6

## Building and running

To build the application, run the following command:

```shell
west build --build-dir samples/sdcard/build -p always -b sh400_bl5340/nrf5340/cpuapp/ns
```
Once you have built the application, run the following command to flash it:

```shell
west flash
```
