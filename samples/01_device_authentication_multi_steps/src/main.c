/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define CERTIFICATE_ZONE 0U
#define PRIVATE_KEY_SLOT 0U
#define CERTIFICATE_BUFFER_SIZE 1024U
#define MAX_CHALLENGE_SIZE 66U
#define MAX_SIGNATURE_SIZE 132U

int main(void)
{
	uint8_t certificate[CERTIFICATE_BUFFER_SIZE];
	uint8_t challenge[MAX_CHALLENGE_SIZE];
	uint8_t signature[MAX_SIGNATURE_SIZE];
	stse_certificate_t ca;
	stse_certificate_t leaf;
	stse_Handler_t *handler;
	stse_ecc_key_type_t key_type;
	stse_ReturnCode_t status;
	uint16_t certificate_size;
	uint16_t challenge_size;
	uint16_t signature_size;

	printk("STSEphyr: multi-step device authentication\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_certificate_parse(stsephyr_st_root_ca, &ca, NULL);
	if (stsephyr_sample_status("parse ST root CA", status) != 0) {
		goto out;
	}

	status = stse_get_device_certificate_size(handler, CERTIFICATE_ZONE,
						  &certificate_size);
	if (stsephyr_sample_status("read leaf certificate size", status) != 0) {
		goto out;
	}
	if (certificate_size > sizeof(certificate)) {
		printk("FAIL: certificate needs %u bytes; buffer has %u\n",
		       certificate_size, (unsigned int)sizeof(certificate));
		goto out;
	}

	status = stse_get_device_certificate(handler, CERTIFICATE_ZONE,
					     certificate_size, certificate);
	if (stsephyr_sample_status("read leaf certificate", status) != 0) {
		goto out;
	}
	printk("Leaf certificate: %u bytes\n", certificate_size);

	status = stse_certificate_parse(certificate, &leaf, NULL);
	if (stsephyr_sample_status("parse leaf certificate", status) != 0) {
		goto out;
	}
	status = stse_certificate_is_parent(&ca, &leaf, NULL);
	if (stsephyr_sample_status("verify leaf certificate against ST root CA", status) != 0) {
		goto out;
	}

	key_type = stse_certificate_get_key_type(&leaf);
	challenge_size = stse_ecc_info_table[key_type].private_key_size;
	signature_size = stse_ecc_info_table[key_type].signature_size;
	if ((challenge_size > sizeof(challenge)) || (signature_size > sizeof(signature))) {
		printk("FAIL: unsupported certificate key sizes (%u/%u)\n",
		       challenge_size, signature_size);
		goto out;
	}
	printk("Certificate key type: %u; challenge: %u bytes; signature: %u bytes\n",
	       key_type, challenge_size, signature_size);

	stsephyr_sample_random(challenge, challenge_size);
	stsephyr_sample_hex("Host challenge", challenge, challenge_size);
	status = stse_ecc_generate_signature(handler, PRIVATE_KEY_SLOT, key_type,
					     challenge, challenge_size, signature);
	if (stsephyr_sample_status("sign challenge in STSAFE-A120", status) != 0) {
		goto out;
	}

	status = stse_certificate_verify_signature(&leaf, challenge, challenge_size,
						   signature, signature_size / 2U,
						   &signature[signature_size / 2U],
						   signature_size / 2U);
	if (stsephyr_sample_status("verify challenge signature on host", status) != 0) {
		goto out;
	}

	stsephyr_sample_close();
	stsephyr_sample_pass("01_device_authentication_multi_steps");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
