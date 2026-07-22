# Porting X-NUCLEO-ESE01A1 to another Zephyr MCU

STSEphyr is not tied to the STM32L452. The driver needs only:

- an I2C controller connected to the STSAFE-A120;
- one GPIO for reset management;
- a Zephyr console for the examples; and
- PSA Crypto with a production-quality entropy source.

Board-specific details belong in devicetree. The driver and example source
normally do not need to change.

## Hardware connections

| Function | X-NUCLEO-ESE01A1 connection |
| --- | --- |
| SDA | Arduino D14/SDA |
| SCL | Arduino D15/SCL |
| Reset management | Arduino A5 through the shield's inverting PMOS |
| Power | Compatible board supply and common ground |
| Serial log | Target board's Zephyr console, usually the debugger virtual COM port |

The standard evaluation profile uses the 7-bit I2C address `0x20`. Confirm the
address if the device has been personalized. Before powering the boards, also
check voltage compatibility, I2C pull-ups, solder bridges, connector
orientation, and pin conflicts.

Start at 100 kHz for bring-up. Change to 400 kHz only after the basic example
is reliable and the pull-ups and bus capacitance are suitable.

## Arduino-compatible boards

The existing shield overlay uses Zephyr's standard `arduino_i2c` and
`arduino_header` definitions. On an Arduino UNO R3-compatible Zephyr board,
first try it directly:

```console
west build -p always -d build/<board>-basic \
  -b <zephyr-board-name> --shield x_nucleo_ese01a1 \
  stsephyr/samples/basic
```

For example, Nucleo-G474RE compiles without any additional overlay:

```console
west build -p always -d build/g474-basic \
  -b nucleo_g474re --shield x_nucleo_ese01a1 \
  stsephyr/samples/basic
```

Zephyr already configures its Arduino I2C bus on PB8/PB9, console, HSI48, and
hardware RNG. This configuration has been compile-tested, but not yet run on a
physical G474 board.

After building, inspect `build/<board>-basic/zephyr/zephyr.dts`. Verify that:

- `stsafe-a120@20` is enabled below the expected I2C controller;
- SDA, SCL, clock frequency, and reset GPIO match the board schematic; and
- the I2C, GPIO, console, RNG, and their required clocks are enabled.

If only pin routing or an RNG clock differs, add a small board-specific shield
overlay at:

```text
boards/shields/x_nucleo_ese01a1/boards/<zephyr-board-name>.overlay
```

The existing `nucleo_l452re.overlay` is an example of this approach.

## Custom board or direct wiring

For a board without Zephyr Arduino connector definitions, describe the
STSAFE-A120 below the actual I2C controller. Replace the pins in this example
with the target's real wiring:

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/dt-bindings/i2c/i2c.h>

&i2c2 {
	pinctrl-0 = <&i2c2_scl_pb10 &i2c2_sda_pb11>;
	pinctrl-names = "default";
	clock-frequency = <I2C_BITRATE_STANDARD>;
	status = "okay";

	stsafe_a120: stsafe-a120@20 {
		compatible = "st,stsafe-a120";
		reg = <0x20>;
		reset-gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
		status = "okay";
	};
};
```

Use `GPIO_ACTIVE_HIGH` when driving the X-NUCLEO-ESE01A1 A5 input because its
PMOS inverts the signal. For a direct connection to the STSAFE-A120 reset pin,
use the polarity of the actual circuit, normally `GPIO_ACTIVE_LOW`.

Build with the new overlay while developing it:

```console
west build -p always -d build/custom-basic \
  -b <zephyr-board-name> stsephyr/samples/basic -- \
  -DDTC_OVERLAY_FILE=C:/path/to/stsafe-a120.overlay
```

The binding is documented in `dts/bindings/crypto/st,stsafe-a120.yaml`.

## Entropy and host cryptography

STSELib uses host cryptography for certificate verification, ECDH, and other
operations. `CONFIG_STSEPHYR` enables PSA Crypto and requests a CSPRNG, but the
board must provide real entropy.

Enable the MCU hardware RNG and its source clock when supported. Otherwise use
a production-grade Zephyr entropy driver for that platform. Do not qualify
authentication or key-management examples with a predictable test RNG.

## Safe first run

Flash only the non-destructive basic example initially:

```console
west flash -d build/<board>-basic --context
west flash -d build/<board>-basic
```

Open the console at 115200 8N1. A successful first boot ends with:

```text
<inf> stsephyr: stsafe-a120@20 ready at 0x20 on <i2c-controller>
<inf> stsephyr_sample: STSAFE-A120 echo successful
PASS: basic
```

Then qualify the board in this order:

1. Repeat `basic` after reset and cold boot.
2. Run random, hash, command-access audit, and storage-read examples.
3. Run device authentication and record its log and duration.
4. Run the safe Twister hardware scenarios.

Do not enable any `CONFIG_SAMPLE_STSAFE_ALLOW_*` option during board bring-up.
Persistent scenarios are connectivity tests only at compile time and remain
`build_only: true` in Twister.

Common initialization failures are usually caused by the wrong reset polarity,
an incorrect I2C address, missing pull-ups, disabled pinctrl/clocks, or a console
configured on another UART.

## Add the board to Twister

After the physical board passes the safe tests:

1. Add its Zephyr name to `platform_allow` and `integration_platforms` in only
   the sample `sample.yaml` files that were qualified.
2. Generate a hardware map:

   ```console
   west twister --generate-hardware-map hardware-map-<board>.yml
   ```

3. Review its board, probe ID, serial port, baud rate, and runner. Add the
   `stsafe_a120` fixture:

   ```yaml
   - connected: true
     fixtures:
       - stsafe_a120
     id: <debug-probe-id>
     platform: <zephyr-board-name>
     runner: <working-runner>
     serial: <serial-port>
     baud: 115200
   ```

4. Start with the basic scenario:

   ```console
   west twister -T stsephyr/samples --device-testing \
     --hardware-map hardware-map-<board>.yml \
     -s sample.stsephyr.basic --inline-logs --short-build-path -ll DEBUG
   ```

Twister automatically skips every scenario marked `build_only`. Once the safe
suite passes, record the wiring, board revision, Zephyr version, flash runner,
serial port, results, and timings in the support matrix and validation log.
