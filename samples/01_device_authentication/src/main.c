/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define CERTIFICATE_ZONE 0U
#define PRIVATE_KEY_SLOT 0U

int main(void)
{
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	printk("STSEphyr: device authentication\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_device_authenticate(handler, stsephyr_st_root_ca,
					  CERTIFICATE_ZONE, PRIVATE_KEY_SLOT);
	stsephyr_sample_close();
	if (stsephyr_sample_status("certificate-chain and challenge authentication", status) != 0) {
		return 0;
	}

	stsephyr_sample_pass("01_device_authentication");
	return 0;
}
