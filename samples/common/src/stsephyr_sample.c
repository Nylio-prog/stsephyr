/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>

static const struct device *const stsafe = DEVICE_DT_GET_ONE(st_stsafe_a120);

static const char *access_condition_name(uint8_t access_condition)
{
	switch (access_condition) {
	case STSE_AC_ALWAYS:
		return "ALWAYS";
	case STSE_AC_HOST:
		return "HOST";
	case STSE_AC_AUTH_AND_HOST:
		return "AUTH + HOST";
	default:
		return "NEVER";
	}
}

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

	printk(" - Initialize target STSAFE-A120 through the Zephyr device model: SUCCESS\n");
	return 0;
}

void stsephyr_sample_close(void)
{
	stsephyr_release(stsafe);
}

void stsephyr_sample_banner(const char *title, const char *description)
{
	printk("\n---------------------------------------------------------------------------------"
	       "-------------------------------\n");
	printk("  %s\n", title);
	printk("-----------------------------------------------------------------------------------"
	       "-----------------------------\n");
	if ((description != NULL) && (description[0] != '\0')) {
		printk("  %s\n", description);
		printk("---------------------------------------------------------------------------"
		       "-------------------------------------\n");
	}
}

void stsephyr_sample_section(const char *title)
{
	printk("\n## %s\n", title);
}

void stsephyr_sample_hex(const char *label, const uint8_t *data, size_t length)
{
	printk("%s (%u bytes):", label, (unsigned int)length);
	for (size_t i = 0; i < length; ++i) {
		if ((i % 16U) == 0U) {
			printk("\n ");
		}
		printk(" 0x%02X", data[i]);
	}
	printk("\n");
}

static int hex_nibble(char value)
{
	if ((value >= '0') && (value <= '9')) {
		return value - '0';
	}
	if ((value >= 'a') && (value <= 'f')) {
		return value - 'a' + 10;
	}
	if ((value >= 'A') && (value <= 'F')) {
		return value - 'A' + 10;
	}

	return -EINVAL;
}

int stsephyr_sample_hex_decode(const char *hex, uint8_t *data, size_t length)
{
	if ((hex == NULL) || (data == NULL) || (strlen(hex) != (length * 2U))) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < length; ++i) {
		int high = hex_nibble(hex[i * 2U]);
		int low = hex_nibble(hex[(i * 2U) + 1U]);

		if ((high < 0) || (low < 0)) {
			return -EINVAL;
		}
		data[i] = (uint8_t)((high << 4) | low);
	}

	return 0;
}

void stsephyr_sample_partition_table(const stsafea_data_partition_record_t *partitions,
				     uint8_t partition_count)
{
	printk("\n - stse_get_data_partitions_configuration\n");
	printk(" ID  | COUNTER | DATA SIZE | READ AC CR | READ AC     | UPDATE AC CR | "
	       "UPDATE AC   | COUNTER\n");
	printk("-----+---------+-----------+------------+-------------+--------------+"
	       "-------------+--------\n");
	for (uint8_t i = 0U; i < partition_count; ++i) {
		const stsafea_data_partition_record_t *partition = &partitions[i];

		printk(" %03u |    %c    |   %04u    |  %-7s   | %-11s |   %-7s    | "
		       "%-11s | %06u\n",
		       partition->index, partition->zone_type == 0U ? '.' : 'x',
		       partition->data_segment_length,
		       partition->read_ac_cr == 1U ? "ALLOWED" : "DENIED",
		       access_condition_name(partition->read_ac),
		       partition->update_ac_cr == 1U ? "ALLOWED" : "DENIED",
		       access_condition_name(partition->update_ac),
		       (unsigned int)partition->counter_value);
	}
}

void stsephyr_sample_random(uint8_t *data, size_t length)
{
	sys_rand_get(data, length);
}

int stsephyr_sample_status(const char *operation, stse_ReturnCode_t status)
{
	if (status == STSE_OK) {
		printk(" - %s: SUCCESS\n", operation);
		return 0;
	}

	printk("FAIL: %s returned STSELib status 0x%04X\n", operation, status);
	return stsephyr_stse_to_errno(status);
}

void stsephyr_sample_footer(void)
{
	printk("\n*#*# STMICROELECTRONICS #*#*\n");
}

void stsephyr_sample_pass(const char *name)
{
	printk("PASS: %s\n", name);
}
