/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define MAX_COMMAND_RECORDS 64U

static const char *access_condition_name(stse_cmd_access_conditions_t condition)
{
	switch (condition) {
	case STSE_CMD_AC_NEVER:
		return "NEVER";
	case STSE_CMD_AC_FREE:
		return "FREE";
	case STSE_CMD_AC_ADMIN:
		return "ADMIN";
	case STSE_CMD_AC_HOST:
		return "HOST";
	case STSE_CMD_AC_ADMIN_OR_PWD:
		return "ADMIN_OR_PWD";
	case STSE_CMD_AC_ADMIN_OR_HOST:
		return "ADMIN_OR_HOST";
	default:
		return "UNKNOWN";
	}
}

int main(void)
{
	stse_cmd_authorization_record_t records[MAX_COMMAND_RECORDS];
	stse_cmd_authorization_CR_t change_rights;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint8_t record_count;

	stsephyr_sample_banner(
		"STSAFE-A120 command access-condition audit",
		"Reads command authorization and host-encryption requirements without changing "
		"the device configuration.");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_device_get_command_count(handler, &record_count);
	if (stsephyr_sample_status("stse_device_get_command_count", status) != 0) {
		goto out;
	}
	if (record_count > MAX_COMMAND_RECORDS) {
		printk("FAIL: device reports %u command records; example supports %u\n",
		       record_count, MAX_COMMAND_RECORDS);
		goto out;
	}

	status = stse_device_get_command_AC_records(handler, record_count, &change_rights, records);
	if (stsephyr_sample_status("stse_device_get_command_AC_records", status) != 0) {
		goto out;
	}

	printk("Command access-condition change right: %u\n", change_rights.cmd_AC_CR);
	printk("Host-encryption change right: %u\n", change_rights.host_encryption_flag_CR);
	printk("Command records: %u\n", record_count);
	printk(" Header | Extended | Access condition | Encrypt command | Encrypt response\n");
	for (uint8_t i = 0U; i < record_count; ++i) {
		printk("  0x%02X  |   0x%02X   | %-16s |        %u        |        %u\n",
		       records[i].header, records[i].extended_header,
		       access_condition_name(records[i].command_AC),
		       records[i].host_encryption_flags.cmd, records[i].host_encryption_flags.rsp);
	}

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("02_command_access_conditions");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
