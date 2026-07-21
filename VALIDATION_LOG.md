# STSEphyr category-01 validation

Environment: Zephyr v4.4.0, Zephyr SDK 1.0.1, west 1.5.0, STSELib v1.1.9,
`nucleo_l452re` plus `x_nucleo_ese01a1`.

Each application below was built with:

```console
west build -p always -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/<sample>
```

All 12 application images (the project template plus the 11 category-01
examples) linked successfully. The individual build reports contained no
compiler or linker errors. The resulting images are standard Zephyr ELF/HEX
artifacts and are flashed with `west flash -r openocd`.

The connected host exposed the ST-LINK virtual COM port as `COM3`, but the
ST-LINK debug USB interface reported Windows device-manager error 28. An
OpenOCD flash attempt therefore reached the Zephyr runner and stopped at
`LIBUSB_ERROR_NOT_FOUND`; no on-target PASS logs could be collected until the
ST-LINK debug driver is installed or the probe is replaced. This is a host
probe issue, not a build failure. The safe storage defaults were left enabled:
zone updates and counter decrements were not attempted.

Once the debug interface is available, flash each image and capture the
115200-8-N-1 console. Every sample emits a final line of the form
`PASS: <sample-name>` after its STSAFE-A120 operations complete.
