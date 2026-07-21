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

	printk("STSEphyr: STSAFE-A120 random number\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_generate_random(handler, random, sizeof(random));
	stsephyr_sample_close();
	if (stsephyr_sample_status("generate random bytes in STSAFE-A120", status) != 0) {
		return 0;
	}
	stsephyr_sample_hex("Random output", random, sizeof(random));
	stsephyr_sample_pass("01_random_number");
	return 0;
}
