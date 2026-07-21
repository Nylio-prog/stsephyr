/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define MESSAGE_SIZE 128U
#define SHA256_SIZE 32U

int main(void)
{
	uint8_t message[MESSAGE_SIZE];
	uint8_t host_hash[SHA256_SIZE];
	uint8_t device_hash[SHA256_SIZE];
	uint16_t hash_length = SHA256_SIZE;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	printk("STSEphyr: SHA-256 comparison\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}
	stsephyr_sample_random(message, sizeof(message));

	status = stse_platform_hash_compute(STSE_SHA_256, message, sizeof(message),
					    host_hash, &hash_length);
	if (stsephyr_sample_status("compute SHA-256 through Zephyr PSA backend", status) != 0) {
		goto out;
	}

	hash_length = sizeof(device_hash);
	status = stse_compute_hash(handler, STSE_SHA_256, message, sizeof(message),
				   device_hash, &hash_length);
	if (stsephyr_sample_status("compute SHA-256 in STSAFE-A120", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("SHA-256", device_hash, hash_length);
	if ((hash_length != SHA256_SIZE) ||
	    (memcmp(host_hash, device_hash, SHA256_SIZE) != 0)) {
		printk("FAIL: host and STSAFE-A120 hashes differ\n");
		goto out;
	}

	stsephyr_sample_close();
	stsephyr_sample_pass("01_hash");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
