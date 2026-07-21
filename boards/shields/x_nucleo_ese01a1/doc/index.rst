X-NUCLEO-ESE01A1
================

Overview
********

The X-NUCLEO-ESE01A1 is an Arduino UNO R3 expansion board containing an
STSAFE-A120 with the standard evaluation profile. STSEphyr uses the Arduino I2C
bus and the reset-management input connected to Arduino A5. The standard profile
uses the 7-bit I2C address 0x20.

On Nucleo-L452RE the board-specific shield overlay routes Arduino I2C to
PB8/PB9, enables the HSI48 clock required by the hardware RNG, and accounts for
the shield's inverted A5 reset-management circuit.

Requirements
************

For the initial supported configuration, stack the shield directly on a
Nucleo-L452RE. Keep the board's default power-selection links installed. The
shield includes configurable I2C pull-ups; verify the solder bridges against the
X-NUCLEO-ESE01A1 UM3531 user manual before changing them.

Build
*****

.. code-block:: console

   west build -b nucleo_l452re --shield x_nucleo_ese01a1 stsephyr/samples/basic
