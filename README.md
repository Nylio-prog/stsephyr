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
| `01_device_authentication` | Validates the device certificate chain and proves possession of slot 0 | None |
| `01_device_authentication_multi_steps` | Runs and reports each certificate and challenge-response authentication step | None |
| `01_echo_loop` | Checks five variable-length echo transactions | None |
| `01_hash` | Compares host-side PSA SHA-256 with STSAFE-A120 SHA-256 | None |
| `01_key_pair_generation_NIST_P256` | Generates, signs, and verifies with a NIST P-256 key in slot 1 | Generates a key pair |
| `01_key_pair_generation_NIST_P521` | Generates, signs, and verifies with a NIST P-521 key in slot 1 | Generates a key pair |
| `01_key_pair_generation_BRAINPOOL_P512` | Generates, signs, and verifies with a Brainpool P-512 key in slot 1 | Generates a key pair |
| `01_key_pair_generation_EDWARDS_25519` | Generates, signs, and verifies with an Ed25519 key in slot 1 | Generates a key pair |
| `01_random_number` | Reads 64 random bytes from the STSAFE-A120 TRNG | None |
| `01_secure_data_storage_counter_access` | Discovers and reads the configured counter zone | Counter decrement is disabled |
| `01_secure_data_storage_zone_access` | Discovers and reads the configured data zone | Zone update is disabled |
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

## Validation

Build the configuration that enables every supported optional STSEphyr feature:

```shell
west build -p always -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/basic -- \
  -DEXTRA_CONF_FILE=stsephyr/samples/basic/full.conf
```

Run the sample build matrix and host-side tests with Twister:

```shell
west twister -T stsephyr/samples -T stsephyr/tests --inline-logs
```

On Windows, use a short Twister output path (for example, `-O C:\\twister-out`)
if the workspace is nested deeply enough to reach the Windows object-path limit.

See [TECHNICAL_DETAILS.md](TECHNICAL_DETAILS.md) for architecture, limitations,
the validation strategy, and hardware notes.

## Safety

The introductory sample does not provision keys, change the I2C address, alter
access conditions, or permanently lock the secure element. Applications using
such commands must implement their own authorization and lifecycle controls.

## License

STSEphyr is licensed under Apache-2.0. STSELib remains under its upstream
BSD-3-Clause license.
