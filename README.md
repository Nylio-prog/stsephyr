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

## Examples

The `samples/` directory contains independent Zephyr applications that cover
authentication, cryptography, storage, key management, and policy inspection.

| Sample | What it demonstrates | Persistent STSAFE side effects by default |
| --- | --- | --- |
| `basic` | Initializes STSELib through Zephyr and performs one echo command | None |
| `01_device_authentication` | Prints the raw and parsed device certificate, validates its chain, and proves possession of slot 0 | None |
| `01_device_authentication_multi_steps` | Prints both parsed certificates, the host challenge, and the STSAFE signature while reporting every authentication step | None |
| `01_echo_loop` | Prints and checks both sides of five variable-length echo transactions | None |
| `01_hash` | Prints the input and compares the host-side PSA and STSAFE-A120 SHA-256 values | None |
| `01_random_number` | Reads 64 random bytes from the STSAFE-A120 TRNG | None |
| `01_key_pair_generation` | Generates P-256, P-521, or Brainpool P-512 key pairs | Disabled; replacing a persistent asymmetric-key slot requires explicit opt-in |
| `01_secure_data_storage_counter_access` | Prints the partition table, configured counter-zone data, and current counter | Counter decrement is disabled |
| `01_secure_data_storage_zone_access` | Prints the partition table and 100 bytes from the configured data zone | Zone update is disabled |
| `02_command_access_conditions` | Audits command authorization and command-encryption settings | None; read-only |
| `02_host_key_provisioning` | Provisions host MAC and cipher keys in plaintext or wrapped form | Disabled; replacing persistent host keys requires explicit opt-in |
| `03_ecdh` | Performs authenticated ephemeral ECDH and compares the host/device shared secrets | None, but matching host session keys must already be provisioned |
| `03_key_wrapping` | Wraps and unwraps an ephemeral key | Wrap-key generation is disabled because it replaces a persistent slot |
| `04_symmetric_key_control_fields` | Audits or updates symmetric-key access controls | Audit is read-only; persistent policy update requires explicit opt-in |
| `05_symmetric_key_operations` | Exercises CMAC and CCM with established or wrapped AES keys | Disabled; replacing a persistent symmetric-key slot requires explicit opt-in |
| `project_template` | Minimal starting point for a customer application | None |

Build any of them with the same board and shield arguments, for example:

```shell
west build -p always -d build/01_device_authentication \
  -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/01_device_authentication
west flash -d build/01_device_authentication
```

### Persistent-operation safeguards

Every operation that changes persistent STSAFE-A120 state is disabled by
default and requires a specific Kconfig opt-in:

| Kconfig option | Persistent change |
| --- | --- |
| `CONFIG_SAMPLE_STSAFE_ALLOW_ZONE_UPDATE` | Overwrites bytes in the selected data zone |
| `CONFIG_SAMPLE_STSAFE_ALLOW_COUNTER_DECREMENT` | Consumes counter value; the decrement cannot be undone |
| `CONFIG_SAMPLE_STSAFE_ALLOW_KEY_PAIR_GENERATION` | Replaces the selected asymmetric private key and configures its usage limit |
| `CONFIG_SAMPLE_STSAFE_ALLOW_HOST_KEY_PROVISIONING` | Replaces the host MAC and cipher keys used for authenticated/encrypted sessions |
| `CONFIG_SAMPLE_STSAFE_ALLOW_WRAP_KEY_GENERATION` | Replaces the selected key-wrapping key |
| `CONFIG_SAMPLE_STSAFE_ALLOW_SYMMETRIC_CONTROL_UPDATE` | Changes persistent access controls for the selected symmetric-key slot |
| `CONFIG_SAMPLE_STSAFE_ALLOW_SYMMETRIC_KEY_WRITE` | Replaces the selected symmetric key and its metadata |

The corresponding Twister scenarios are marked `build_only: true`. CI compiles
the enabled code paths, but a normal hardware run cannot flash them. Review the
sample's Kconfig help and source, confirm the target slot and current lifecycle
state, and use only a device whose contents may be replaced before manually
building and flashing one of these applications.

The wrapping and symmetric-key samples authenticate a host session before
issuing their persistent write, so invalid host keys fail before the selected
slot is touched. This preflight cannot apply to host-key provisioning itself,
because that operation intentionally installs new host keys.

Host keys are deliberately not stored in this repository. Put them in a local,
untracked configuration file when compiling a sample that needs them, for
example:

```ini
CONFIG_SAMPLE_STSAFE_HOST_MAC_KEY_HEX="00112233445566778899aabbccddeeff"
CONFIG_SAMPLE_STSAFE_HOST_CIPHER_KEY_HEX="00112233445566778899aabbccddeeff"
```

Pass that file with `-DEXTRA_CONF_FILE=C:/path/to/private.conf` and never commit
production key material. Host-key provisioning also requires
`CONFIG_SAMPLE_STSAFE_ALLOW_HOST_KEY_PROVISIONING=y` and a plaintext or wrapped
provisioning mode. It does not change the provisioning-control fields for you.

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

Twister builds and flashes each hardware-enabled application, skips scenarios
marked `build_only`, streams its 115200-baud serial
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

To move X-NUCLEO-ESE01A1 to another Zephyr MCU—including the compile-validated
Nucleo-G474RE configuration—follow [PORTING.md](PORTING.md). It covers both
Arduino-compatible boards and custom wiring, devicetree overlays, entropy,
safe bring-up, and Twister hardware-map qualification.

## Safety

Safe sample variants do not provision keys, change the I2C address, alter
access conditions, decrement counters, or permanently lock the secure element.
State-changing variants require an explicit Kconfig gate and are compile-only
in Twister. They must be reviewed and flashed manually on a suitably
provisioned or disposable device; permanent-lock scenarios are not provided.

## License

STSEphyr is licensed under Apache-2.0. STSELib remains under its upstream
BSD-3-Clause license.
