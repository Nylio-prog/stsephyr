/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define KEY_SLOT 1U
#define USAGE_LIMIT 255U
#define KEY_TYPE STSE_ECC_KT_ED25519
#define EXAMPLE_NAME "01_key_pair_generation_EDWARDS_25519"

#define HASH_SIZE(size) ((size) - ((size) % 16U))

int main(void)
{
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint8_t verification_status = 0U;
	const uint16_t message_size = HASH_SIZE(stse_ecc_info_table[KEY_TYPE].min_signature_message_size);
	uint8_t public_key[stse_ecc_info_table[KEY_TYPE].public_key_size];
	uint8_t message[message_size];
	uint8_t signature[stse_ecc_info_table[KEY_TYPE].signature_size];

	printk("STSEphyr: STSAFE-A120 Edwards 25519 key-pair generation\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_generate_ecc_key_pair(handler, KEY_SLOT, KEY_TYPE, USAGE_LIMIT, public_key);
	if (stsephyr_sample_status("generate Ed25519 key pair in slot 1", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Public key", public_key, sizeof(public_key));

	/* Ed25519 uses the message directly (eddsa_variant = 0), not a digest. */
	stsephyr_sample_random(message, sizeof(message));
	status = stse_ecc_generate_signature(handler, KEY_SLOT, KEY_TYPE, message,
					     sizeof(message), signature);
	if (stsephyr_sample_status("sign message with Ed25519 private key", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Signature", signature, sizeof(signature));

	status = stse_ecc_verify_signature(handler, KEY_TYPE, public_key, signature,
					   message, sizeof(message), 0U, &verification_status);
	if (stsephyr_sample_status("verify Ed25519 signature in STSAFE-A120", status) != 0 ||
	    verification_status != 1U) {
		printk("FAIL: Ed25519 signature verification was invalid\n");
		goto out;
	}

	stsephyr_sample_close();
	stsephyr_sample_pass(EXAMPLE_NAME);
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
