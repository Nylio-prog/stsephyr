/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define SECRET_SIZE	    16U
#define WRAPPED_SECRET_SIZE (SECRET_SIZE + 8U)

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
	uint8_t plaintext[SECRET_SIZE];
	uint8_t wrapped[WRAPPED_SECRET_SIZE] = {0};
	uint8_t unwrapped[SECRET_SIZE] = {0};
	stsafea_aes_128_host_keys_t host_keys;
	stse_session_t session;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 local-envelope wrapping example",
		"Wraps and unwraps a random secret using a configured device-local AES key.");

#if !defined(CONFIG_SAMPLE_STSAFE_ENABLE_KEY_WRAPPING)
	printk("SKIPPED: key wrapping is disabled until matching host keys are supplied.\n");
	stsephyr_sample_footer();
	stsephyr_sample_pass("03_key_wrapping_safe_skip");
	return 0;
#endif

	if (load_host_keys(&host_keys) != 0) {
		return 0;
	}
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}
	stsafea_session_clear_context(&session);
	status = stsafea_open_host_session(handler, &session, host_keys.host_mac_key,
					   host_keys.host_cipher_key);
	if (stsephyr_sample_status("preflight host-session authentication", status) != 0) {
		goto out;
	}
	stsafea_close_host_session(&session);
	printk(" - Supplied host keys match the provisioned device keys\n");

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_WRAP_KEY_GENERATION)
	printk("WARNING: opt-in enabled; local-envelope slot %d will be generated or "
	       "replaced.\n",
	       CONFIG_SAMPLE_STSAFE_WRAP_KEY_SLOT);
	status = stsafea_generate_wrap_unwrap_key(handler, CONFIG_SAMPLE_STSAFE_WRAP_KEY_SLOT,
						  STSE_AES_128_KT);
	if (stsephyr_sample_status("stsafea_generate_wrap_unwrap_key", status) != 0) {
		goto out;
	}
#else
	printk("Using the existing local-envelope key in slot %d; no key is generated.\n",
	       CONFIG_SAMPLE_STSAFE_WRAP_KEY_SLOT);
#endif

	stsephyr_sample_random(plaintext, sizeof(plaintext));
	stsephyr_sample_hex("Plaintext secret", plaintext, sizeof(plaintext));
	stsafea_session_clear_context(&session);
	status = stsafea_open_host_session(handler, &session, host_keys.host_mac_key,
					   host_keys.host_cipher_key);
	if (stsephyr_sample_status("stsafea_open_host_session", status) != 0) {
		goto out;
	}

	status = stsafea_wrap_payload(handler, CONFIG_SAMPLE_STSAFE_WRAP_KEY_SLOT, plaintext,
				      sizeof(plaintext), wrapped, sizeof(wrapped));
	if (stsephyr_sample_status("stsafea_wrap_payload", status) != 0) {
		stsafea_close_host_session(&session);
		goto out;
	}
	stsephyr_sample_hex("Wrapped secret", wrapped, sizeof(wrapped));
	status = stsafea_unwrap_payload(handler, CONFIG_SAMPLE_STSAFE_WRAP_KEY_SLOT, wrapped,
					sizeof(wrapped), unwrapped, sizeof(unwrapped));
	stsafea_close_host_session(&session);
	if (stsephyr_sample_status("stsafea_unwrap_payload", status) != 0) {
		goto out;
	}
	if (memcmp(plaintext, unwrapped, sizeof(plaintext)) != 0) {
		printk("FAIL: unwrapped secret differs from the original\n");
		goto out;
	}
	printk(" - Wrapped payload round trip: SUCCESS\n");

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("03_key_wrapping");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
