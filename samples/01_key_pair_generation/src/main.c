/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define MAX_PUBLIC_KEY_SIZE 132U
#define MAX_SIGNATURE_SIZE  132U
#define MAX_HASH_SIZE	    64U

#if defined(CONFIG_SAMPLE_STSAFE_KEY_CURVE_NIST_P521)
#define SAMPLE_KEY_TYPE	  STSE_ECC_KT_NIST_P_521
#define SAMPLE_CURVE_NAME "NIST P-521"
#elif defined(CONFIG_SAMPLE_STSAFE_KEY_CURVE_BRAINPOOL_P512)
#define SAMPLE_KEY_TYPE	  STSE_ECC_KT_BP_P_512
#define SAMPLE_CURVE_NAME "Brainpool P-512"
#else
#define SAMPLE_KEY_TYPE	  STSE_ECC_KT_NIST_P_256
#define SAMPLE_CURVE_NAME "NIST P-256"
#endif

int main(void)
{
	uint8_t public_key[MAX_PUBLIC_KEY_SIZE];
	uint8_t signature[MAX_SIGNATURE_SIZE];
	uint8_t hash[MAX_HASH_SIZE];
	const stse_ecc_info_t *key_info = &stse_ecc_info_table[SAMPLE_KEY_TYPE];
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint8_t verification = 0U;
	uint16_t hash_length = key_info->min_signature_message_size;

	stsephyr_sample_banner(
		"STSAFE-A120 asymmetric key-pair generation example",
		"Generates a persistent ECC private key, signs a random digest and verifies "
		"the signature.");
	printk("Target slot: %d\n", CONFIG_SAMPLE_STSAFE_KEY_SLOT);
	printk("Curve: %s\n", SAMPLE_CURVE_NAME);
	printk("Usage limit: %d\n", CONFIG_SAMPLE_STSAFE_KEY_USAGE_LIMIT);

#if !defined(CONFIG_SAMPLE_STSAFE_ALLOW_KEY_PAIR_GENERATION)
	printk("SKIPPED: persistent key-pair generation is disabled.\n");
	printk("Enabling it replaces slot %d and may consume its configured usage limit.\n",
	       CONFIG_SAMPLE_STSAFE_KEY_SLOT);
	stsephyr_sample_footer();
	stsephyr_sample_pass("01_key_pair_generation_safe_skip");
	return 0;
#endif

	if ((key_info->public_key_size > sizeof(public_key)) ||
	    (key_info->signature_size > sizeof(signature)) || (hash_length > sizeof(hash))) {
		printk("FAIL: selected curve exceeds the example buffer limits\n");
		return 0;
	}

	printk("WARNING: opt-in enabled; slot %d will be permanently replaced.\n",
	       CONFIG_SAMPLE_STSAFE_KEY_SLOT);
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	stsephyr_sample_section("Generate the persistent ECC key pair");
	status = stse_generate_ecc_key_pair(handler, CONFIG_SAMPLE_STSAFE_KEY_SLOT, SAMPLE_KEY_TYPE,
					    CONFIG_SAMPLE_STSAFE_KEY_USAGE_LIMIT, public_key);
	if (stsephyr_sample_status("stse_generate_ecc_key_pair", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Generated public key", public_key, key_info->public_key_size);

	stsephyr_sample_random(hash, hash_length);
	stsephyr_sample_hex("Digest to sign", hash, hash_length);
	status = stse_ecc_generate_signature(handler, CONFIG_SAMPLE_STSAFE_KEY_SLOT,
					     SAMPLE_KEY_TYPE, hash, hash_length, signature);
	if (stsephyr_sample_status("stse_ecc_generate_signature", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Signature", signature, key_info->signature_size);

	status = stse_ecc_verify_signature(handler, SAMPLE_KEY_TYPE, public_key, signature, hash,
					   hash_length, 0U, &verification);
	if (stsephyr_sample_status("stse_ecc_verify_signature", status) != 0) {
		goto out;
	}
	if (verification != 1U) {
		printk("FAIL: generated signature was not verified\n");
		goto out;
	}

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("01_key_pair_generation");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
