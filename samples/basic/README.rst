STSEphyr basic sample
####################

This sample initializes the STSAFE-A120 on X-NUCLEO-ESE01A1 through STSELib and
performs a non-destructive echo command. It never changes personalization,
access conditions, keys, lifecycle state, or the I2C address.

Build and run on the supported Nucleo-L452RE host:

.. code-block:: console

   west build -b nucleo_l452re --shield x_nucleo_ese01a1 stsephyr/samples/basic
   west flash

The expected console message is ``STSAFE-A120 echo successful``.
