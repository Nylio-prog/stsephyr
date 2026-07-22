/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#include <core/stse_platform.h>

#define ECDH_KEY_TYPE		STSE_ECC_KT_NIST_P_256
#define ECDH_EPHEMERAL_SLOT	0xFFU
#define ECDH_PUBLIC_KEY_SIZE	64U
#define ECDH_PRIVATE_KEY_SIZE	32U
#define ECDH_SHARED_SECRET_SIZE 32U

static int load_host_keys(stsafea_aes_128_host_keys_t *keys)
{
	if ((stsephyr_sample_hex_decode(CONFIG_SAMPLE_STSAFE_HOST_MAC_KEY_HEX, keys->host_mac_key,
					STSAFEA_HOST_AES_128_MAC_KEY_SIZE) != 0) ||
	    (stsephyr_sample_hex_decode(CONFIG_SAMPLE_STSAFE_HOST_CIPHER_KEY_HEX,
					keys->host_cipher_key,
					STSAFEA_HOST_AES_128_CIPHER_KEY_SIZE) != 0)) {
		printk("FAIL: both provisioned host keys must be supplied as 32 hex characters\n");
		return -1;
	}

	return 0;
}

int main(void)
{
	uint8_t host_private_key[ECDH_PRIVATE_KEY_SIZE];
	uint8_t host_public_key[ECDH_PUBLIC_KEY_SIZE];
	uint8_t device_public_key[ECDH_PUBLIC_KEY_SIZE];
	uint8_t host_secret[ECDH_SHARED_SECRET_SIZE];
	uint8_t device_secret[ECDH_SHARED_SECRET_SIZE];
	stsafea_aes_128_host_keys_t host_keys;
	stse_session_t session;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 ephemeral ECDH example",
		"Generates ephemeral NIST P-256 key pairs on the host and secure element and "
		"compares their shared secrets.");

#if !defined(CONFIG_SAMPLE_STSAFE_ENABLE_ECDH)
	printk("SKIPPED: ECDH is disabled until matching provisioned host keys are supplied.\n");
	stsephyr_sample_footer();
	stsephyr_sample_pass("03_ecdh_safe_skip");
	return 0;
#endif

	if (load_host_keys(&host_keys) != 0) {
		return 0;
	}
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_platform_ecc_generate_key_pair(ECDH_KEY_TYPE, host_private_key,
						     host_public_key);
	if (stsephyr_sample_status("stse_platform_ecc_generate_key_pair", status) != 0) {
		goto out;
	}
	status = stsafea_generate_ecc_key_pair(handler, ECDH_EPHEMERAL_SLOT, ECDH_KEY_TYPE, 1U,
					       device_public_key);
	if (stsephyr_sample_status("stsafea_generate_ecc_key_pair(ephemeral)", status) != 0) {
		goto out;
	}

	stsafea_session_clear_context(&session);
	status = stsafea_open_host_session(handler, &session, host_keys.host_mac_key,
					   host_keys.host_cipher_key);
	if (stsephyr_sample_status("stsafea_open_host_session", status) != 0) {
		goto out;
	}

	status = stse_ecc_establish_shared_secret(handler, ECDH_EPHEMERAL_SLOT, ECDH_KEY_TYPE,
						  host_public_key, device_secret);
	if (stsephyr_sample_status("stse_ecc_establish_shared_secret", status) != 0) {
		stsafea_close_host_session(&session);
		goto out;
	}
	stsafea_close_host_session(&session);

	status = stse_platform_ecc_ecdh(ECDH_KEY_TYPE, device_public_key, host_private_key,
					host_secret);
	if (stsephyr_sample_status("stse_platform_ecc_ecdh", status) != 0) {
		goto out;
	}
	if (memcmp(host_secret, device_secret, sizeof(host_secret)) != 0) {
		printk("FAIL: host and STSAFE-A120 shared secrets differ\n");
		goto out;
	}
	printk(" - Host and STSAFE-A120 shared secrets match\n");

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("03_ecdh");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
