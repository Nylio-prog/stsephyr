/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define RANDOM_SIZE 64U

int main(void)
{
	uint8_t random[RANDOM_SIZE];
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 Random number generation example",
		"Generates 64 random bytes using the STSAFE-A120 random-number command.");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_generate_random(handler, random, sizeof(random));
	if (stsephyr_sample_status("stse_generate_random (64 bytes)", status) != 0) {
		goto out;
	}
	stsephyr_sample_section("STSAFE-A120 random output");
	stsephyr_sample_hex("Random data", random, sizeof(random));
	stsephyr_sample_footer();
	stsephyr_sample_close();
	stsephyr_sample_pass("01_random_number");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
