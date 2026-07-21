# STSEphyr

STSEphyr is the STMicroelectronics Zephyr port for the STSAFE-A120 secure
element. It integrates the upstream
[STSELib](https://github.com/STMicroelectronics/STSELib) middleware without
forking or copying it.

The initial supported hardware is:

- Zephyr 4.4.0;
- STSELib 1.1.9;
- STM32 Nucleo-L452RE host board;
- X-NUCLEO-ESE01A1 expansion board with its default STSAFE-A120 evaluation
  profile.

## Workspace setup

Install the Zephyr prerequisites, the Zephyr SDK, and west as described in the
[Zephyr getting-started guide](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html).
Install STM32CubeProgrammer with its ST-LINK USB driver for flashing. Then
create a workspace from this manifest repository:

```shell
mkdir stsephyr-workspace
cd stsephyr-workspace
git clone https://github.com/Nylio-prog/stsephyr.git stsephyr
west init -l stsephyr
west update
west zephyr-export
west packages pip --install
```

STSELib is checked out by west at `modules/lib/stselib` and remains an external
dependency at its own pinned revision.

## Build the basic sample

Stack X-NUCLEO-ESE01A1 on the Nucleo-L452RE and build:

```shell
west build -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/basic
west flash
```

Open the ST-LINK virtual COM port at 115200 8N1. A successful run initializes
STSELib and performs a non-destructive echo command.

## SDK-inspired examples

Category-01 examples inspired by the STSAFE-A SDK are provided as independent
Zephyr applications under `samples/`.

| Sample | What it demonstrates | Persistent STSAFE side effects by default |
| --- | --- | --- |
| `basic` | Initializes STSELib through Zephyr and performs one echo command | None |
| `01_device_authentication` | Prints the raw and parsed device certificate, validates its chain, and proves possession of slot 0 | None |
| `01_device_authentication_multi_steps` | Prints both parsed certificates, the host challenge, and the STSAFE signature while reporting every authentication step | None |
| `01_echo_loop` | Prints and checks both sides of five variable-length echo transactions | None |
| `01_hash` | Prints the input and compares the host-side PSA and STSAFE-A120 SHA-256 values | None |
| `01_random_number` | Reads 64 random bytes from the STSAFE-A120 TRNG | None |
| `01_secure_data_storage_counter_access` | Prints the partition table, configured counter-zone data, and current counter | Counter decrement is disabled |
| `01_secure_data_storage_zone_access` | Prints the partition table and 100 bytes from the configured data zone | Zone update is disabled |
| `project_template` | Minimal starting point for a customer application | None |

Build any of them with the same board and shield arguments, for example:

```shell
west build -p always -d build/01_device_authentication \
  -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/01_device_authentication
west flash -d build/01_device_authentication
```

Storage writes and counter decrements are disabled by default. Enable the
corresponding sample Kconfig option explicitly only when using a disposable,
provisioned device.

Key-pair-generation examples are intentionally excluded because generating a
key changes persistent slot state and can consume its configured usage limit.

## Validation

Build the configuration that enables every supported optional STSEphyr feature:

```shell
west build -p always -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/basic -- \
  -DEXTRA_CONF_FILE=stsephyr/samples/basic/full.conf
```

Run the host-side unit tests and compile the hardware sample matrix:

```shell
west twister -T stsephyr/tests --inline-logs -O twister-out-tests
west twister -T stsephyr/samples --build-only --inline-logs \
  --short-build-path -O twister-out-samples
```

GitHub Actions uses the same split: host-side tests execute normally, while the
samples are compilation-only because its runners do not have an STSAFE-A120
attached.

Run every sample as a hardware integration test with the Nucleo-L452RE and
X-NUCLEO-ESE01A1 connected:

```shell
west twister -T stsephyr/samples --device-testing \
  --hardware-map stsephyr/hardware-map.yml --inline-logs \
  --short-build-path -ll DEBUG
```

Twister builds and flashes each application, streams its 115200-baud serial
output as `DEVICE:` debug messages, and passes the test when the expected
`PASS: <sample>` marker appears. It reports pass/fail status and execution
duration for every scenario, with each complete serial transcript in the
scenario's `handler.log` and machine-readable results in `twister-out`. Omit
`-ll DEBUG` when only the concise progress and result summary is needed.

The checked-in `hardware-map.yml` identifies the current Nucleo by its ST-LINK
probe ID, uses the OpenOCD flash runner, maps its virtual serial port to `COM3`,
and advertises the required `stsafe_a120` fixture. If Windows assigns another
port, update the `serial` field before running the tests. To regenerate a map
for another probe, run:

```shell
west twister --generate-hardware-map stsephyr/hardware-map.yml
```

Then restore the generated entry's `platform`, `runner`, and `fixtures` fields
as shown in the checked-in map. To run only one example, select its Twister
scenario, for example:

```shell
west twister -T stsephyr/samples --device-testing \
  --hardware-map stsephyr/hardware-map.yml \
  -s sample.stsephyr.hash --inline-logs --short-build-path -ll DEBUG
```

On Windows, add `--short-build-path` to Twister commands that build the samples.
Some of the generated Mbed TLS object paths can exceed the Windows path limit
even when a short output directory is used.

See [TECHNICAL_DETAILS.md](TECHNICAL_DETAILS.md) for architecture, limitations,
the validation strategy, and hardware notes.

## Safety

The introductory sample does not provision keys, change the I2C address, alter
access conditions, or permanently lock the secure element. Applications using
such commands must implement their own authorization and lifecycle controls.

## License

STSEphyr is licensed under Apache-2.0. STSELib remains under its upstream
BSD-3-Clause license.
