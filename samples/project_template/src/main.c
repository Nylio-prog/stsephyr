/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

int main(void)
{
	stse_Handler_t *handler;

	printk("STSEphyr: customer project template\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	/*
	 * Call STSELib APIs with handler while the device is acquired. Keep the
	 * critical section short: STSELib v1.1.9 platform state is serialized.
	 */

	stsephyr_sample_close();
	stsephyr_sample_pass("project_template");
	return 0;
}
