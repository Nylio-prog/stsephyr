/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>

static const struct device *const stsafe = DEVICE_DT_GET_ONE(st_stsafe_a120);

int stsephyr_sample_open(stse_Handler_t **handler)
{
	int ret;

	if (!device_is_ready(stsafe)) {
		printk("FAIL: STSAFE-A120 device is not ready\n");
		return -ENODEV;
	}

	ret = stsephyr_acquire(stsafe, K_MSEC(CONFIG_STSEPHYR_LOCK_TIMEOUT_MS), handler);
	if (ret != 0) {
		printk("FAIL: cannot acquire STSAFE-A120 (%d)\n", ret);
		return ret;
	}

	printk("STSAFE-A120 ready through Zephyr device model\n");
	return 0;
}

void stsephyr_sample_close(void)
{
	stsephyr_release(stsafe);
}

void stsephyr_sample_hex(const char *label, const uint8_t *data, size_t length)
{
	printk("%s (%u bytes):", label, (unsigned int)length);
	for (size_t i = 0; i < length; ++i) {
		if ((i % 16U) == 0U) {
			printk("\n  ");
		}
		printk("%02x ", data[i]);
	}
	printk("\n");
}

void stsephyr_sample_random(uint8_t *data, size_t length)
{
	sys_rand_get(data, length);
}

int stsephyr_sample_status(const char *operation, stse_ReturnCode_t status)
{
	if (status == STSE_OK) {
		printk("OK: %s\n", operation);
		return 0;
	}

	printk("FAIL: %s returned STSELib status 0x%04x\n", operation, status);
	return stsephyr_stse_to_errno(status);
}

void stsephyr_sample_pass(const char *name)
{
	printk("PASS: %s\n", name);
}
