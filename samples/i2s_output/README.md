# Demonstration application
Firmware to output a sound wave from memory over I2S.

## Building and running

To build the application, run the following command:

```shell
west build --build-dir samples/i2s_output/build -p always -b sh400_bl5340/nrf5340/cpuapp/ns
```
Once you have built the application, run the following command to flash it:

```shell
west flash
```
