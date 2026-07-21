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
artifacts and are flashed with `west flash`.

Plain `west flash` was verified with STM32CubeProgrammer v2.23.0 and the
NUCLEO-L452RE ST-LINK interface. The basic image was programmed through SWD,
verified, reset, and started successfully. Its 115200-8-N-1 console output was:

```text
<inf> stsephyr: stsafe-a120@20 ready at 0x20 on i2c@40005400
*** Booting Zephyr OS build v4.4.0 ***
<inf> stsephyr_sample: STSAFE-A120 echo successful
```

The safe storage defaults were left enabled: zone updates and counter
decrements were not attempted. Category-01 images were compile-validated; a
complete release qualification should additionally flash each one and record
its final `PASS: <sample-name>` line on the target device.
