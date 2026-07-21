/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define MESSAGE_SIZE 128U
#define SHA256_SIZE  32U

int main(void)
{
	uint8_t message[MESSAGE_SIZE];
	uint8_t host_hash[SHA256_SIZE];
	uint8_t device_hash[SHA256_SIZE];
	uint16_t hash_length = SHA256_SIZE;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 hash commands example",
		"Computes SHA-256 on the host and in STSAFE-A120, then compares both results.");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}
	stsephyr_sample_random(message, sizeof(message));
	stsephyr_sample_section("Message buffer to hash");
	stsephyr_sample_hex("Message buffer", message, sizeof(message));

	status = stse_platform_hash_compute(STSE_SHA_256, message, sizeof(message), host_hash,
					    &hash_length);
	if (stsephyr_sample_status("stse_platform_hash_compute", status) != 0) {
		goto out;
	}
	stsephyr_sample_section("Platform SHA-256");
	stsephyr_sample_hex("Platform hash", host_hash, hash_length);

	hash_length = sizeof(device_hash);
	status = stse_compute_hash(handler, STSE_SHA_256, message, sizeof(message), device_hash,
				   &hash_length);
	if (stsephyr_sample_status("stse_compute_hash", status) != 0) {
		goto out;
	}
	stsephyr_sample_section("STSAFE-A120 SHA-256");
	stsephyr_sample_hex("STSAFE-A120 hash", device_hash, hash_length);
	if ((hash_length != SHA256_SIZE) || (memcmp(host_hash, device_hash, SHA256_SIZE) != 0)) {
		printk("FAIL: host and STSAFE-A120 hashes differ\n");
		goto out;
	}
	printk("\n - HASH SUCCESS: platform and STSAFE-A120 results match\n");
	stsephyr_sample_footer();

	stsephyr_sample_close();
	stsephyr_sample_pass("01_hash");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
