# STSEphyr category-01 validation

Environment: Zephyr v4.4.0, Zephyr SDK 1.0.1, west 1.5.0, STSELib v1.1.9,
`nucleo_l452re` plus `x_nucleo_ese01a1`.

Each application below was built with:

```console
west build -p always -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/<sample>
```

All eight application images (the project template plus the seven
safe-by-default category-01 examples) linked successfully. The individual
build reports contained no compiler or linker errors. The resulting images are
standard Zephyr ELF/HEX artifacts and are flashed with `west flash`.

Plain `west flash` was verified with STM32CubeProgrammer v2.23.0 and the
NUCLEO-L452RE ST-LINK interface. The basic image was programmed through SWD,
verified, reset, and started successfully. Its 115200-8-N-1 console output was:

```text
<inf> stsephyr: stsafe-a120@20 ready at 0x20 on i2c@40005400
*** Booting Zephyr OS build v4.4.0 ***
<inf> stsephyr_sample: STSAFE-A120 echo successful
```

## Category-01 hardware run

On 2026-07-21, each image was flashed through the normal Zephyr runner and run
on the connected NUCLEO-L452RE plus X-NUCLEO-ESE01A1. The 115200-8-N-1 console
reported the following final results:

| Application | Observed result |
| --- | --- |
| `project_template` | `PASS: project_template` |
| `01_device_authentication` | Printed the raw DER and parsed device certificate, then verified the certificate chain and slot-0 proof of possession |
| `01_device_authentication_multi_steps` | Printed the parsed production CA and device certificates, the host challenge, and the STSAFE signature; all verification steps passed |
| `01_echo_loop` | Five variable-length request/response buffers matched |
| `01_hash` | The printed host and STSAFE-A120 SHA-256 values matched |
| `01_random_number` | Printed 64 bytes returned by the STSAFE-A120 random service |
| `01_secure_data_storage_counter_access` | Printed the partition table, 16 associated-data bytes, and the counter value; decrement was skipped |
| `01_secure_data_storage_zone_access` | Printed the partition table and 100 data-zone bytes; update was skipped |

Every application emitted its expected `PASS: <sample-name>` line. Immediate
Zephyr logging mode was used by these applications so the large certificate
and buffer dumps reached the UART without deferred-log message loss.

The safe storage defaults were left enabled: no zone update or counter
decrement was attempted. Key-pair-generation applications are deliberately not
provided or tested because their operations alter persistent key-slot state.
