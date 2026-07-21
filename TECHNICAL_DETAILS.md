# STSEphyr technical details

## 1. Scope and supported baseline

STSEphyr is a Zephyr adaptation layer for STSELib and STSAFE-A120. It keeps
[STSELib](https://github.com/STMicroelectronics/STSELib) as an external,
independently versioned dependency. STSEphyr does not copy or reimplement the
STSAFE command, frame, service, or certificate layers.

The initial supported tuple is:

- Zephyr v4.4.0;
- Zephyr SDK 1.0.1 or another toolchain accepted by Zephyr 4.4.0;
- STSELib v1.1.9;
- NUCLEO-L452RE;
- X-NUCLEO-ESE01A1 with its default STSAFE-A120 evaluation profile;
- I2C communication at the speed described by the host controller.

The evaluation profile permits all STSAFE-A120 commands, so the port does not
maintain a command allowlist. All STSAFE-A command and API sources required by
STSELib are compiled. This is separate from application policy: destructive
commands such as provisioning, locking, key replacement, access-condition
changes, and I2C-address changes must still be explicitly controlled by the
product application.

STSAFE-L, ST1Wire, and non-Zephyr hosts are out of scope. STSAFE-A110 has not
been qualified and is not advertised as supported.

## 2. Architecture and ownership

```text
Zephyr application
    |
    | stsephyr_acquire() / stsephyr_release()
    v
STSELib v1.1.9 public API using stse_Handler_t
    |
    | STSELib Platform Abstraction Layer callbacks
    v
Zephyr I2C, GPIO, kernel timing, CSPRNG, and PSA Crypto
    |
    v
NUCLEO-L452RE -- Arduino R3 -- X-NUCLEO-ESE01A1 -- STSAFE-A120
```

STSELib owns command encoding, response parsing, protocol sequencing, secure
sessions, device services, and certificates. STSEphyr owns:

- west dependency pinning and Zephyr module discovery;
- the reviewed STSAFE-A-only source list;
- Kconfig-to-STSELib configuration translation;
- devicetree binding and X-NUCLEO shield description;
- Zephyr device lifecycle and serialized access;
- I2C, reset, delay, CRC, random, and host-crypto PAL callbacks;
- samples, tests, documentation, and CI.

The public API deliberately exposes the upstream `stse_Handler_t`. A second
wrapper API for every secure-element operation would duplicate STSELib, obscure
upstream documentation, and create another compatibility surface.

## 3. Dependency and build integration

`west.yml` pins Zephyr and STSELib by release tag. Zephyr's imported manifest is
filtered to the modules needed by this target: CMSIS, CMSIS 6, STM32 HAL,
Mbed TLS, and TF-PSA-Crypto. A consumer with an existing west workspace may add
STSEphyr and STSELib as projects instead of using the standalone manifest.

`zephyr/module.yml` exposes the repository's CMake, Kconfig, board, and
devicetree roots. The build expects STSELib at `modules/lib/stselib`, or at an
explicit `STSELIB_ROOT` supplied by the parent build.

STSELib v1.1.9 has no Zephyr module metadata and requires the integrating
project to provide `stse_conf.h` and `stse_platform_generic.h`. STSEphyr provides
both without changing the dependency checkout. CMake names every selected
STSELib source explicitly; it does not use a recursive glob that could silently
pull in STSAFE-L or new upstream files after an upgrade.

No patch is applied to STSELib. `stse_platform_generic.h` maps its platform
integer, packed-structure, and CMSIS-style weak-symbol conventions onto Zephyr.
`stse_conf.h` enables only STSAFE-A and maps selected algorithms and optional
host helpers from Kconfig.

## 4. Hardware and devicetree model

X-NUCLEO-ESE01A1 is modeled as the `x_nucleo_ese01a1` Zephyr shield, not as a
custom board. The common overlay creates one `st,stsafe-a120` child on
`arduino_i2c` with 7-bit address `0x20`. The reset-management signal is connected
through Arduino A5. The shield's P-channel MOSFET inverts the STSAFE-A120's
active-low reset input, so the MCU-side A5 command is declared active-high. On
NUCLEO-L452RE, A5 resolves to PC0.

The NUCLEO-L452RE base devicetree assigns I2C1 SDA to PB7, while the Arduino
D14/SDA connector used by the shield is PB9. The board-specific shield overlay
therefore selects PB8/PB9 explicitly. It also enables both the STM32 hardware
RNG and its required HSI48 domain clock because the PAL requires a
cryptographically secure random source. Enabling the RNG without HSI48 blocks
STM32 entropy initialization before the UART console starts.

The X-NUCLEO board has configurable I2C pull-ups and supply links; these must be
set consistently with the host voltage as described in ST's
[UM3531](https://www.st.com/resource/en/user_manual/um3531-how-to-use-stm32-nucleo-expansion-board-based-on-the-stsafea120-secure-element-stmicroelectronics.pdf).

The binding keeps bus, address, and reset routing in devicetree. The C driver
does not hard-code `0x20`, so a differently personalized STSAFE-A120 can be
described by changing `reg` in an application overlay. Multiple devicetree
instances are structurally supported, subject to `CONFIG_STSEPHYR_MAX_INSTANCES`.

## 5. Device lifecycle

Each enabled `st,stsafe-a120` node creates one Zephyr device containing an
STSELib handler, an I/O assembly buffer, transaction state, and an instance
mutex. Initialization runs at `POST_KERNEL` and performs these steps:

1. verify that the I2C and reset GPIO controllers are ready;
2. configure reset inactive, then assert it for the configured interval;
3. register the small numeric bus ID used by STSELib callbacks;
4. initialize an STSELib handler with `stse_set_default_handler_value()`;
5. set type `STSAFE_A120`, devicetree address, bus speed, and I2C bus type;
6. call `stse_init()` and verify that the resulting device type is A120;
7. mark the Zephyr device ready only after all prior steps succeed.

`stsephyr_reset()` repeats the hardware reset and handler initialization while
holding the normal library locks. Initialization failures leave the device
unavailable instead of exposing a partially initialized handler.

## 6. Concurrency and memory model

Applications must call `stsephyr_acquire()`, complete the entire sequence of
STSELib calls that belongs to one logical operation, then call
`stsephyr_release()`. The acquire operation locks both the instance and a global
library mutex.

Global serialization is intentional in v1.1.9: the upstream PAL contract uses
process-global CRC state, and the streaming CMAC adaptation also has operation
state shared across callbacks. A per-device mutex alone would therefore allow
two instances to corrupt each other's cryptographic or frame state. This can be
relaxed only after an upstream-version review proves all PAL state is
per-handler.

STSAFE-A120's maximum STSE frame is 752 bytes. CRC and length overhead can make
a PAL receive transaction 756 bytes, so every instance reserves a 756-byte I/O
buffer. Segmented send and receive callbacks track the declared total and
current offset, reject overflow, and require the stop offset to equal the
declared length. They do not allocate large frame buffers on thread stacks.

## 7. Platform callback mapping

| STSELib service | Zephyr implementation | Important behavior |
| --- | --- | --- |
| I2C init/send/receive | `i2c_dt_spec`, `i2c_write_dt()`, `i2c_read_dt()` | Uses the devicetree bus and address; translates NACK, arbitration, timeout, and generic bus errors. |
| Segmented I/O | Per-instance 756-byte buffer | Validates every segment and exact final length. |
| Reset/power | `gpio_dt_spec` | Active-low polarity is handled by Zephyr's GPIO descriptor. Power callbacks control reset; the expansion board supply is not switched. |
| Delay | `k_msleep()` | Does not busy-wait during STSAFE processing intervals. |
| Random | `sys_csrand_get()` | Initialization fails if a CSPRNG is unavailable; NUCLEO-L452RE enables its hardware RNG. |
| CRC16 | Local CRC-16/X-25 implementation | Supports calculate and accumulate callbacks; known vector `123456789` is `0x906e`. |
| Host cryptography | PSA Crypto | Supplies hash, ECC, ECDSA, ECDH, CMAC, AES, HMAC/HKDF, and RFC 3394 AES key wrap needed by enabled STSELib features. |

The legacy monolithic STSELib receive callback is rejected because the A120
configuration uses the bounded segmented callback family. The wake callback
uses a zero-length I2C write; its electrical behavior remains part of physical
hardware qualification because controller implementations may differ.

## 8. Algorithms and configuration

The default algorithm set enables NIST P-256, NIST P-384, SHA-256, SHA-384, and
SHA-512. Optional Kconfig selections exist for P-521, Brainpool curves, and
Curve25519 when a product profile requires them and its PSA backend supports
them. Ed25519 is intentionally not exposed because the v1.1.9 callback set and
current PSA adapter do not provide a complete, verified implementation.

Host secure sessions, host-key establishment, wrapped provisioning, and
symmetric-key establishment helpers are separately selectable and default off.
They are not needed for non-authenticated commands. Enabling a helper compiles
the relevant STSELib path but never provisions a key automatically; the product
must define secure key storage and lifecycle policy.

The PSA adapter destroys transient imported keys after each operation. It never
logs keys, secrets, plaintext provisioning payloads, or complete frames. Debug
logging reports only provider status values.

## 9. Public API and error handling

Typical application use is:

```c
const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(stsafe_a120));
stse_Handler_t *handler;

if (stsephyr_acquire(dev, K_SECONDS(5), &handler) == 0) {
        stse_ReturnCode_t status = stse_device_echo(handler);

        stsephyr_release(dev);
        /* Handle status after releasing the lock. */
}
```

The wrapper returns normal negative Zephyr `errno` values for lifecycle and
locking failures. STSELib operations continue to return `stse_ReturnCode_t` so
their full protocol meaning is preserved. `stsephyr_stse_to_errno()` is provided
for callers that need coarse translation: invalid parameters become `-EINVAL`,
buffer overflow `-EMSGSIZE`, receive timeout `-ETIMEDOUT`, authorization failure
`-EACCES`, missing entries `-ENOENT`, and other failures `-EIO`.

## 10. Verification

Every sample is a Twister console-harness scenario that requires the
`stsafe_a120` fixture and matches its exact `PASS: <sample>` marker. The
checked-in hardware map identifies the Nucleo-L452RE, its ST-LINK serial port
and OpenOCD runner, and the attached fixture. The CRC ztest checks both one-shot
and incremental calculation against the standard X-25 vector. CI uses the
official Zephyr setup action, executes the host-side test, compiles the hardware
samples with `--build-only`, and rejects whitespace errors. It cannot execute
the sample harnesses because GitHub-hosted runners have no STSAFE-A120.

The initial implementation has been compile-validated with Zephyr v4.4.0,
Zephyr SDK 1.0.1, STSELib v1.1.9, board `nucleo_l452re`, and shield
`x_nucleo_ese01a1`. The linked basic image used 54,216 bytes of flash and 15,616
bytes of RAM in that configuration. These numbers are indicative and can change
with compiler, Kconfig, logging, and dependency updates.

The full optional-feature build also links successfully and used 112,900 bytes
of flash and 25,344 bytes of RAM in the same environment. Twister discovered
and built both sample configurations. On Windows, use Twister's
`--short-build-path` option; a short output directory alone is insufficient for
the longest generated Mbed TLS object paths.

Physical qualification still requires the actual two-board stack. At minimum,
record successful reset/initialization, echo, repeated commands, maximum-length
transfer, polling timeout behavior, concurrent caller serialization, and
repeated reset. The introductory test must not run irreversible commands even
though the default evaluation profile allows them.

## 11. STSAFE-A SDK-inspired category-01 samples

The `samples/` tree mirrors the non-provisioning category-01 projects from the
STSAFE-A SDK. Each sample is a normal Zephyr application and shares a small
helper that acquires the STSAFE device, reports STSELib status, and prints a
machine-readable `PASS:` line:

| Sample | Demonstrates | Persistent side effects by default |
| --- | --- | --- |
| `01_device_authentication` | Raw DER and parsed device-certificate output followed by chain and challenge authentication with the ST production CA | None |
| `01_device_authentication_multi_steps` | Parsed CA/device certificates, host challenge, device signature, and each host-verification step | None |
| `01_echo_loop` | Full variable-length request and response buffers (bounded to 1--500 bytes) | None |
| `01_hash` | Input buffer plus host PSA and STSAFE SHA-256 results | None |
| `01_random_number` | 64 bytes from the STSAFE random service | None |
| `01_secure_data_storage_zone_access` | Full partition table and 100-byte data-zone readback | Updates are disabled unless explicitly opted in |
| `01_secure_data_storage_counter_access` | Full partition table, associated data, and counter-zone readback | Decrement is disabled unless explicitly opted in |

The applications select Zephyr's immediate logging mode. Certificate and
variable-length buffer dumps can otherwise fill the deferred logging queue and
produce `messages dropped` reports before the UART backend drains it.

The project template is a minimal customer starting point. Build any sample
with the same board/shield arguments, for example:

```console
west build -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/01_hash
```

For destructive SDK-parity operations, opt in deliberately at build time and
record the device state before/after the run:

```console
west build -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/01_secure_data_storage_zone_access -- \
  -DCONFIG_SAMPLE_STSAFE_ALLOW_ZONE_UPDATE=y
west build -b nucleo_l452re --shield x_nucleo_ese01a1 \
  stsephyr/samples/01_secure_data_storage_counter_access -- \
  -DCONFIG_SAMPLE_STSAFE_ALLOW_COUNTER_DECREMENT=y
```

### Flashing through Zephyr runners

The NUCLEO-L452RE board definition selects STM32CubeProgrammer as its default
Zephyr flash runner. After installing STM32CubeProgrammer and its ST-LINK USB
driver, flash the most recent build with:

```console
west flash
```

For a named build directory, use `west flash -d build/<sample>`. Zephyr passes
the generated HEX file to STM32CubeProgrammer, programs it through SWD, resets
the MCU, and starts the application. No HEX copying or programmer-specific
command is needed. `west flash --context` displays the selected runner and
arguments; OpenOCD, J-Link, and ST-LINK GDB server remain optional alternatives.

## 12. Known limitations and upgrade rules

- Only NUCLEO-L452RE plus X-NUCLEO-ESE01A1 is in the initial support matrix.
- No Zephyr generic crypto-device API is provided; applications use STSELib.
- Runtime power removal is not supported; the PAL power hooks only control reset.
- Hardware wake signaling and all-command behavior require on-target validation.
- Global serialization limits concurrency across multiple STSAFE instances.
- Provisioning and product-specific key storage are deliberately outside the
  basic sample and driver policy.

An STSELib update is not just a tag change. Maintainers must review callback
signatures, `stse_conf.h` symbols, the explicit source list, maximum frame sizes,
handler layout, global PAL state, and return codes, then rebuild all Kconfig
variants and repeat the hardware smoke test. A Zephyr update must additionally
review the binding schema, connector labels, board qualifiers, PSA wants, GPIO
semantics, and SDK/toolchain baseline. Dependency checkouts must remain
unmodified and release manifests must continue to pin immutable revisions.

## 13. References

- [STSELib v1.1.9](https://github.com/STMicroelectronics/STSELib/tree/v1.1.9)
- [Zephyr external modules](https://docs.zephyrproject.org/4.4.0/develop/modules.html)
- [Zephyr shield porting](https://docs.zephyrproject.org/4.4.0/hardware/porting/shields.html)
- [Zephyr devicetree bindings](https://docs.zephyrproject.org/4.4.0/build/dts/bindings.html)
- [Zephyr Twister](https://docs.zephyrproject.org/4.4.0/develop/test/twister.html)
- [X-NUCLEO-ESE01A1 product page](https://www.st.com/en/evaluation-tools/x-nucleo-ese01a1.html)
- [X-NUCLEO-ESE01A1 user manual UM3531](https://www.st.com/resource/en/user_manual/um3531-how-to-use-stm32-nucleo-expansion-board-based-on-the-stsafea120-secure-element-stmicroelectronics.pdf)
- [CATIE STSAFE-A1xx Zephyr prior art](https://github.com/catie-aq/zephyr_st-stsafe-a1xx)
