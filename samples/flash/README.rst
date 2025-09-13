.. zephyr:code-sample:: soc-flash-nrf
   :name: nRF SoC Internal Storage
   :relevant-api: flash_interface flash_area_api

   Use the flash API to interact with the SoC flash.

Overview
********

This sample demonstrates using the :ref:`Flash API <flash_api>` on an SoC internal storage.
The sample uses :ref:`Flash map API <flash_map_api>` to obtain a device that has one
partition defined with the label ``storage_partition``, then uses :ref:`Flash API <flash_api>`
to directly access and modify the contents of a device within the area defined for said
partition.

Within the sample, user may observe how read/write/erase operations
are performed on a device, and how to first check whether device is
ready for operation.

Building and Running
********************

The sample will be built for any SoC with internal storage, as long as
there is a fixed-partition named ``storage_partition`` defined
on that internal storage.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/soc_flash_nrf
   :board: sh400_bl5340/nrf5340/cpuapp/ns
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting nRF Connect SDK v3.1.0-6c6e5b32496e ***
   *** Using Zephyr OS v4.1.99-1612683d4010 ***

   Nordic nRF5 Internal Storage Sample
   =====================================

   Test 1: Internal storage erase page at 0xfc000
      Erase succeeded!

   Test 2: Internal storage write
         Write block size: 4
         Required write cycles for given data and memory: 32
         Writing 4 data bytes to 0xfc000
         Reading 4 data bytes from 0xfc000
         Writing 4 data bytes to 0xfc004
         Reading 4 data bytes from 0xfc004
         Writing 4 data bytes to 0xfc008
         Reading 4 data bytes from 0xfc008
         Writing 4 data bytes to 0xfc00c
         Reading 4 data bytes from 0xfc00c
         Writing 4 data bytes to 0xfc010
         Reading 4 data bytes from 0xfc010
         Writing 4 data bytes to 0xfc014
         Reading 4 data bytes from 0xfc014
         Writing 4 data bytes to 0xfc018
         Reading 4 data bytes from 0xfc018
         Writing 4 data bytes to 0xfc01c
         Reading 4 data bytes from 0xfc01c
         Writing 4 data bytes to 0xfc020
         Reading 4 data bytes from 0xfc020
         Writing 4 data bytes to 0xfc024
         Reading 4 data bytes from 0xfc024
         Writing 4 data bytes to 0xfc028
         Reading 4 data bytes from 0xfc028
         Writing 4 data bytes to 0xfc02c
         Reading 4 data bytes from 0xfc02c
         Writing 4 data bytes to 0xfc030
         Reading 4 data bytes from 0xfc030
         Writing 4 data bytes to 0xfc034
         Reading 4 data bytes from 0xfc034
         Writing 4 data bytes to 0xfc038
         Reading 4 data bytes from 0xfc038
         Writing 4 data bytes to 0xfc03c
         Reading 4 data bytes from 0xfc03c
         Writing 4 data bytes to 0xfc040
         Reading 4 data bytes from 0xfc040
         Writing 4 data bytes to 0xfc044
         Reading 4 data bytes from 0xfc044
         Writing 4 data bytes to 0xfc048
         Reading 4 data bytes from 0xfc048
         Writing 4 data bytes to 0xfc04c
         Reading 4 data bytes from 0xfc04c
         Writing 4 data bytes to 0xfc050
         Reading 4 data bytes from 0xfc050
         Writing 4 data bytes to 0xfc054
         Reading 4 data bytes from 0xfc054
         Writing 4 data bytes to 0xfc058
         Reading 4 data bytes from 0xfc058
         Writing 4 data bytes to 0xfc05c
         Reading 4 data bytes from 0xfc05c
         Writing 4 data bytes to 0xfc060
         Reading 4 data bytes from 0xfc060
         Writing 4 data bytes to 0xfc064
         Reading 4 data bytes from 0xfc064
         Writing 4 data bytes to 0xfc068
         Reading 4 data bytes from 0xfc068
         Writing 4 data bytes to 0xfc06c
         Reading 4 data bytes from 0xfc06c
         Writing 4 data bytes to 0xfc070
         Reading 4 data bytes from 0xfc070
         Writing 4 data bytes to 0xfc074
         Reading 4 data bytes from 0xfc074
         Writing 4 data bytes to 0xfc078
         Reading 4 data bytes from 0xfc078
         Writing 4 data bytes to 0xfc07c
         Reading 4 data bytes from 0xfc07c
         Data read matches data written. Good!

   Test 3: Internal storage erase (2 pages at 0xfc000)
      Erase succeeded!

   Test 4: Internal storage erase page at 0xfc000
      Erase succeeded!

   Test 5: Non-word aligned write
         Skipping unaligned write, not supported

   Test 6: Page layout API
      Offset  0x00041234:
      belongs to the page 65 of start offset 0x00041000
      and the size of 0x00001000 B.
      Page of number 37 has start offset 0x00025000
      and size of 0x00001000 B.
      Page index resolved properly
      SoC flash consists of 256 pages.

   Test 7: Write block size API
      write-block-size = 4

   Finished!
