/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

static int load_host_keys(stsafea_aes_128_host_keys_t *keys)
{
	if ((stsephyr_sample_hex_decode(CONFIG_SAMPLE_STSAFE_HOST_MAC_KEY_HEX, keys->host_mac_key,
					STSAFEA_HOST_AES_128_MAC_KEY_SIZE) != 0) ||
	    (stsephyr_sample_hex_decode(CONFIG_SAMPLE_STSAFE_HOST_CIPHER_KEY_HEX,
					keys->host_cipher_key,
					STSAFEA_HOST_AES_128_CIPHER_KEY_SIZE) != 0)) {
		printk("FAIL: both host keys must contain exactly 32 hexadecimal characters\n");
		return -1;
	}

	return 0;
}

int main(void)
{
	stsafea_host_key_provisioning_ctrl_fields_t controls;
	stsafea_aes_128_host_keys_t keys;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 host-key provisioning example",
		"Provisions caller-supplied AES-128 host-session keys without changing the "
		"device's provisioning policy.");

#if !defined(CONFIG_SAMPLE_STSAFE_ALLOW_HOST_KEY_PROVISIONING)
	printk("SKIPPED: persistent host-key provisioning is disabled.\n");
	printk("Enabling it replaces the keys used for authenticated and encrypted host "
	       "sessions.\n");
	stsephyr_sample_footer();
	stsephyr_sample_pass("02_host_key_provisioning_safe_skip");
	return 0;
#endif

	if (load_host_keys(&keys) != 0) {
		return 0;
	}
	printk("WARNING: opt-in enabled; the device host keys will be permanently replaced.\n");
	printk("The example will not weaken or lock the current provisioning control fields.\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stsafea_query_host_key_provisioning_ctrl_fields(handler, &controls);
	if (stsephyr_sample_status("stsafea_query_host_key_provisioning_ctrl_fields", status) !=
	    0) {
		goto out;
	}
	printk("Host-key controls: change_right=%u reprovision=%u plaintext=%u "
	       "wrapped_anonymous=%u auth_key=0x%02X\n",
	       controls.change_right, controls.reprovision, controls.plaintext,
	       controls.wrapped_anonymous, controls.wrapped_or_DH_derived_authentication_key);

#if defined(CONFIG_SAMPLE_STSAFE_HOST_KEY_WRAPPED)
	printk("Provisioning method: NIST P-256 wrapped transfer\n");
	status = stse_host_key_provisioning_wrapped(handler, STSAFEA_AES_128_HOST_KEY,
						    (stsafea_host_keys_t *)&keys,
						    STSE_ECC_KT_NIST_P_256);
	if (stsephyr_sample_status("stse_host_key_provisioning_wrapped", status) != 0) {
		goto out;
	}
#else
	printk("Provisioning method: plaintext transfer\n");
	status = stse_host_key_provisioning(handler, STSAFEA_AES_128_HOST_KEY,
					    (stsafea_host_keys_t *)&keys);
	if (stsephyr_sample_status("stse_host_key_provisioning", status) != 0) {
		goto out;
	}
#endif

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("02_host_key_provisioning");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
