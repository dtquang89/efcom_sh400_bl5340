.. _peripheral_uart:

Bluetooth: Peripheral UART
##########################

.. contents::
   :local:
   :depth: 2

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme`.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth® LE connection, emulating a serial port over Bluetooth LE.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires using a compatible application for `Testing`_.
You can use the `Bluetooth Low Energy app`_ for desktop or the `nRF Connect for Mobile`_ (or other similar applications, such as `nRF Blinky`_ or `nRF Toolbox`_).
Using mobile applications for testing requires a smartphone or tablet.

You can also test the application with the :ref:`central_uart` sample.
See the documentation for that sample for detailed instructions.

Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 0 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 0 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 0 peripheral's TX pin.

.. _peripheral_uart_debug:

Debugging
=========

In this sample, the UART console is used to send and read data over the NUS service.
Debug messages are not displayed in this UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

For more information about debugging in the |NCS|, see :ref:`debugging`.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

.. tabs::

   LED 1:
      Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

   LED 2:
      Lit when connected.

   Button 1:
      Confirm the passkey value that is printed in the debug logs to pair/bond with the other device.

   Button 2:
      Reject the passkey value that is printed in the debug logs to prevent pairing/bonding with the other device.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_UART_ASYNC_ADAPTER:

CONFIG_UART_ASYNC_ADAPTER - Enable UART async adapter
   Enables asynchronous adapter for UART drives that supports only IRQ interface.

Building and running
********************

To build the sample, use the following command:

.. code-block:: console

   west build samples/bluetooth/peripheral_uart -b board_name

.. _peripheral_uart_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test the basic functionality:

.. _peripheral_uart_testing_mobile:

Testing with nRF Connect for Mobile
-----------------------------------

You can test the sample pairing with a mobile device.
For this purpose, use `nRF Connect for Mobile`_ (or other similar applications, such as `nRF Blinky`_ or `nRF Toolbox`_).

To perform the test, complete the following steps:

.. tabs::

   .. group-tab:: nRF53

      .. tabs::

         .. group-tab:: Android

            1. Connect the device to the computer to access UART 0.
               If you use a development kit, UART 0 is forwarded as a serial port.
               |serial_port_number_list|
               If you use Thingy:53, you must attach the debug board and connect an external USB to UART converter.
            #. |connect_terminal|
            #. Optionally, you can display debug messages.
               See :ref:`peripheral_uart_debug` for details.
            #. Install and start the `nRF Connect for Mobile`_ application on your Android device or tablet.
            #. If the application does not automatically start scanning, tap the Play icon in the upper right corner.
            #. Connect to the device using nRF Connect for Mobile.

               Observe that **LED 2** is lit.
            #. Optionally, pair or bond with the device with MITM protection.
               This requires using the passkey value displayed in debug messages.
               See :ref:`peripheral_uart_debug` for details on how to access debug messages.
               To confirm pairing or bonding, press **Button 1** on the device and accept the passkey value on the smartphone.
            #. In the application, observe that the services are shown in the connected device.
            #. Select the **UART RX characteristic** option in nRF Connect for Mobile and tap the up arrow button.
               You can write to the UART RX and get the text displayed on the COM listener.
            #. Type "0123456789" and tap :guilabel:`SEND`.

               Verify that the text "0123456789" is displayed on the COM listener.
            #. To send data from the device to your phone or tablet, in the terminal emulator connected to the sample, enter any text, for example, "Hello", and press Enter to see it on the COM listener.

               The text is sent through the development kit to your mobile device over a Bluetooth LE link.
            #. On your Android device or tablet, tap the three-dot menu next to **Disconnect** and select **Show log**.

               The device displays the text in the nRF Connect for Mobile log.
            #. Disconnect the device in nRF Connect for Mobile.

               Observe that **LED 2** turns off.

         .. group-tab:: iOS

            1. Connect the device to the computer to access UART 0.
               If you use a development kit, UART 0 is forwarded as a serial port.
               |serial_port_number_list|
               If you use Thingy:53, you must attach the debug board and connect an external USB to UART converter.
            #. |connect_terminal|
            #. Optionally, you can display debug messages.
               See :ref:`peripheral_uart_debug` for details.
            #. Install and start the `nRF Connect for Mobile`_ application on your iOS device or tablet.
            #. If the application does not automatically start scanning, tap the Play icon in the upper right corner.
            #. Connect to the device using nRF Connect for Mobile.

               Observe that **LED 2** is lit.
            #. Optionally, pair or bond with the device with MITM protection.
               This requires using the passkey value displayed in debug messages.
               See :ref:`peripheral_uart_debug` for details on how to access debug messages.
               To confirm pairing or bonding, press **Button 1** on the device and accept the passkey value on the smartphone.
            #. In the application, observe that the services are shown in the connected device.
            #. Select the **UART RX characteristic** option in nRF Connect for Mobile and tap the up arrow button.

               The **Write Value** window opens.
               You can write to the UART RX and get the text displayed on the COM listener.
            #. Type "0123456789" and tap **Write**.

               Verify that the text "0123456789" is displayed on the COM listener.
            #. To send data from the device to your phone or tablet, in the terminal emulator connected to the sample, enter any text, for example, "Hello", and press Enter to see it on the COM listener.

               The text is sent through the development kit to your mobile device over a Bluetooth LE link.
            #. On your iOS device or tablet, select the **Log** tab.

               The device displays the text in the nRF Connect for Mobile log.
            #. Disconnect the device in nRF Connect for Mobile.

               Observe that **LED 2** turns off.

.. _nrf52_computer_testing:
.. _peripheral_uart_testing_ble:

Testing with Bluetooth Low Energy app
-------------------------------------

If you have an nRF52 Series DK with the Peripheral UART sample and either a dongle or second Nordic Semiconductor development kit that supports the `Bluetooth Low Energy app`_, you can test the sample on your computer.
Use the `Bluetooth Low Energy app`_ in `nRF Connect for Desktop`_ for testing.

To perform the test, complete the following steps:

1. Install the `Bluetooth Low Energy app`_ in `nRF Connect for Desktop`_.
#. Connect to your nRF52 Series DK.
#. Connect the dongle or second development kit to a USB port of your computer.
#. Open the app.
#. Select the serial port that corresponds to the dongle or the second development kit.
   Do not select the kit you want to test just yet.

   .. note::
      If the dongle or the second development kit has not been used with the Bluetooth Low Energy app before, you may be asked to update the J-Link firmware and connectivity firmware on the nRF SoC to continue.
      When the nRF SoC has been updated with the correct firmware, the app finishes connecting to your device over USB.
      When the connection is established, the device appears in the main view.

#. Click :guilabel:`Start scan`.
#. Find the development kit you want to test and click the corresponding :guilabel:`Connect` button.

   The default name for the Peripheral UART sample is *Nordic_UART_Service*.

#. Select the **Universal Asynchronous Receiver/Transmitter (UART)** RX characteristic value.
#. Write ``30 31 32 33 34 35 36 37 38 39`` (the hexadecimal value for the string "0123456789") and click :guilabel:`Write`.

   The data is transmitted over Bluetooth LE from the app to the DK that runs the Peripheral UART sample.
   The terminal emulator connected to the development kit then displays ``"0123456789"``.

#. In the terminal emulator, enter any text, for example ``Hello``.

   The data is transmitted to the development kit that runs the Peripheral UART sample.
   The **UART TX** characteristic displayed in the app changes to the corresponding ASCII value.
   For example, the value for ``Hello`` is ``48 65 6C 6C 6F``.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_uart_async_adapter`
* :ref:`nus_service_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`boards/arm/nrf*/board.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:api_peripherals`:

   * :file:`include/gpio.h`
   * :file:`include/uart.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
