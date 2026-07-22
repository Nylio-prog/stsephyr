/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define MESSAGE_SIZE	     16U
#define TAG_SIZE	     4U
#define ASSOCIATED_DATA_SIZE 8U

#if defined(CONFIG_SAMPLE_STSAFE_SYMMETRIC_ESTABLISH_CCM) ||                                       \
	defined(CONFIG_SAMPLE_STSAFE_SYMMETRIC_WRAPPED_CCM)
#define SAMPLE_USES_CCM 1
#define SAMPLE_KEY_SIZE STSE_AES_256_KEY_SIZE
#else
#define SAMPLE_KEY_SIZE STSE_AES_128_KEY_SIZE
#endif

#if defined(CONFIG_SAMPLE_STSAFE_SYMMETRIC_WRAPPED_CMAC) ||                                        \
	defined(CONFIG_SAMPLE_STSAFE_SYMMETRIC_WRAPPED_CCM)
#define SAMPLE_USES_WRAPPED_PROVISIONING 1
#endif

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

static void configure_key_info(stsafea_generic_key_information_t *key_info)
{
	memset(key_info, 0, sizeof(*key_info));
	key_info->lock_indicator = STSAFEA_SYMMETRIC_KEY_LOCK_INDICATOR_UNLOCKED;
	key_info->slot_number = CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT;
#if defined(SAMPLE_USES_CCM)
	key_info->info_length = STSAFEA_KEY_INFO_LENGTH_CCM;
	key_info->type = STSAFEA_SYMMETRIC_KEY_TYPE_AES_256;
	key_info->mode_of_operation = STSAFEA_KEY_OPERATION_MODE_CCM;
	key_info->usage.encryption = 1U;
	key_info->usage.decryption = 1U;
	key_info->CCM.auth_tag_length = TAG_SIZE;
#else
	key_info->info_length = STSAFEA_KEY_INFO_LENGTH_CMAC;
	key_info->type = STSAFEA_SYMMETRIC_KEY_TYPE_AES_128;
	key_info->mode_of_operation = STSAFEA_KEY_OPERATION_MODE_CMAC;
	key_info->usage.mac_generation = 1U;
	key_info->usage.mac_verification = 1U;
	key_info->CMAC.min_MAC_length = 2U;
#endif
}

#if !defined(SAMPLE_USES_CCM)
static int exercise_cmac(stse_Handler_t *handler)
{
	uint8_t message[MESSAGE_SIZE];
	uint8_t tag[TAG_SIZE];
	uint8_t verified = 0U;
	stse_ReturnCode_t status;

	stsephyr_sample_random(message, sizeof(message));
	status = stse_cmac_hmac_compute(handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, message,
					sizeof(message), tag, sizeof(tag));
	if (stsephyr_sample_status("stse_cmac_hmac_compute", status) != 0) {
		return -1;
	}
	status = stse_cmac_hmac_verify(handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, tag,
				       sizeof(tag), message, sizeof(message), &verified);
	if (stsephyr_sample_status("stse_cmac_hmac_verify", status) != 0) {
		return -1;
	}
	if (verified != 1U) {
		printk("FAIL: CMAC verification failed\n");
		return -1;
	}
	stsephyr_sample_hex("CMAC", tag, sizeof(tag));
	return 0;
}
#endif

#if defined(SAMPLE_USES_CCM)
static int exercise_ccm(stse_Handler_t *handler)
{
	uint8_t message[MESSAGE_SIZE];
	uint8_t encrypted[MESSAGE_SIZE];
	uint8_t decrypted[MESSAGE_SIZE];
	uint8_t tag[TAG_SIZE];
	uint8_t nonce[STSAFEA_NONCE_SIZE];
	uint8_t associated_data[ASSOCIATED_DATA_SIZE];
	uint8_t verified = 0U;
	stse_ReturnCode_t status;

	stsephyr_sample_random(message, sizeof(message));
	stsephyr_sample_random(nonce, sizeof(nonce));
	stsephyr_sample_random(associated_data, sizeof(associated_data));
	status = stse_aes_ccm_encrypt(handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, TAG_SIZE,
				      nonce, sizeof(associated_data), associated_data,
				      sizeof(message), message, encrypted, tag, 0U, NULL);
	if (stsephyr_sample_status("stse_aes_ccm_encrypt", status) != 0) {
		return -1;
	}
	status = stse_aes_ccm_decrypt(handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, TAG_SIZE,
				      nonce, sizeof(associated_data), associated_data,
				      sizeof(encrypted), encrypted, tag, &verified, decrypted);
	if (stsephyr_sample_status("stse_aes_ccm_decrypt", status) != 0) {
		return -1;
	}
	if ((verified != 1U) || (memcmp(message, decrypted, sizeof(message)) != 0)) {
		printk("FAIL: CCM authentication or plaintext comparison failed\n");
		return -1;
	}
	stsephyr_sample_hex("CCM ciphertext", encrypted, sizeof(encrypted));
	stsephyr_sample_hex("CCM authentication tag", tag, sizeof(tag));
	return 0;
}
#endif

int main(void)
{
	uint8_t key[SAMPLE_KEY_SIZE];
	stsafea_generic_key_information_t key_info;
	stsafea_aes_128_host_keys_t host_keys;
	stse_session_t session;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 symmetric-key provisioning and operation example",
		"Writes a selected symmetric-key slot, then verifies a CMAC or CCM operation.");
	printk("Target symmetric-key slot: %d\n", CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT);

#if !defined(CONFIG_SAMPLE_STSAFE_ALLOW_SYMMETRIC_KEY_WRITE)
	printk("SKIPPED: persistent symmetric-key replacement is disabled.\n");
	stsephyr_sample_footer();
	stsephyr_sample_pass("05_symmetric_key_operations_safe_skip");
	return 0;
#endif

	if (load_host_keys(&host_keys) != 0) {
		return 0;
	}
	printk("WARNING: opt-in enabled; key and metadata in slot %d will be replaced.\n",
	       CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT);
	configure_key_info(&key_info);
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

#if defined(SAMPLE_USES_WRAPPED_PROVISIONING)
	memset(key, 0x5AU, sizeof(key));
	status = stse_write_symmetric_key_wrapped(handler, key, &key_info, STSE_ECC_KT_NIST_P_256);
	if (stsephyr_sample_status("stse_write_symmetric_key_wrapped", status) != 0) {
		goto out;
	}
#else
	status = stse_establish_symmetric_key(handler, STSE_ECC_KT_NIST_P_256, 1U, &key_info, key);
	if (stsephyr_sample_status("stse_establish_symmetric_key", status) != 0) {
		goto out;
	}
#endif

	stsafea_session_clear_context(&session);
	status = stsafea_open_host_session(handler, &session, host_keys.host_mac_key,
					   host_keys.host_cipher_key);
	if (stsephyr_sample_status("stsafea_open_host_session", status) != 0) {
		goto out;
	}

#if defined(SAMPLE_USES_CCM)
	if (exercise_ccm(handler) != 0) {
#else
	if (exercise_cmac(handler) != 0) {
#endif
		stsafea_close_host_session(&session);
		goto out;
	}
	stsafea_close_host_session(&session);

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("05_symmetric_key_operations");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
