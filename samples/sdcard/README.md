# mindfield_esense_firmware
Firmware for analysis current consumption of Read/Write SD Card over 24h, using nRF52840 DK board.

### Building and running

To build the application, run the following command:

```shell
west build --build-dir samples/sdcard/build -p always -b mindfield_esense samples/sdcard
```
Once you have built the application, run the following command to flash it:

```shell
west flash
```
