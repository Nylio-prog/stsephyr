/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define ECHO_ITERATIONS 5U
#define ECHO_MAX_LENGTH 500U

int main(void)
{
	uint8_t request[ECHO_MAX_LENGTH];
	uint8_t response[ECHO_MAX_LENGTH];
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint16_t random_length;

	printk("STSEphyr: echo loop (%u iterations)\n", ECHO_ITERATIONS);
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	for (unsigned int iteration = 0; iteration < ECHO_ITERATIONS; ++iteration) {
		stsephyr_sample_random((uint8_t *)&random_length, sizeof(random_length));
		random_length = (random_length % ECHO_MAX_LENGTH) + 1U;
		stsephyr_sample_random(request, random_length);
		memset(response, 0, random_length);

		status = stse_device_echo(handler, request, response, random_length);
		if (status != STSE_OK) {
			stsephyr_sample_status("STSAFE-A120 echo", status);
			goto out;
		}
		if (memcmp(request, response, random_length) != 0) {
			printk("FAIL: echo mismatch on iteration %u\n", iteration + 1U);
			goto out;
		}
		printk("OK: echo %u/%u (%u bytes)\n", iteration + 1U,
		       ECHO_ITERATIONS, random_length);
		k_sleep(K_MSEC(100));
	}

	stsephyr_sample_close();
	stsephyr_sample_pass("01_echo_loop");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
