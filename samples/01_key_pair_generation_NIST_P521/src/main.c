/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define KEY_SLOT 1U
#define USAGE_LIMIT 255U
#define KEY_TYPE STSE_ECC_KT_NIST_P_521
#define EXAMPLE_NAME "01_key_pair_generation_NIST_P521"

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

	printk("STSEphyr: STSAFE-A120 NIST P-521 key-pair generation\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_generate_ecc_key_pair(handler, KEY_SLOT, KEY_TYPE, USAGE_LIMIT, public_key);
	if (stsephyr_sample_status("generate NIST P-521 key pair in slot 1", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Public key", public_key, sizeof(public_key));

	stsephyr_sample_random(message, sizeof(message));
	status = stse_ecc_generate_signature(handler, KEY_SLOT, KEY_TYPE, message,
					     sizeof(message), signature);
	if (stsephyr_sample_status("sign message with NIST P-521 private key", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Signature", signature, sizeof(signature));

	status = stse_ecc_verify_signature(handler, KEY_TYPE, public_key, signature,
					   message, sizeof(message), 0U, &verification_status);
	if (stsephyr_sample_status("verify NIST P-521 signature in STSAFE-A120", status) != 0 ||
	    verification_status != 1U) {
		printk("FAIL: NIST P-521 signature verification was invalid\n");
		goto out;
	}

	stsephyr_sample_close();
	stsephyr_sample_pass(EXAMPLE_NAME);
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
