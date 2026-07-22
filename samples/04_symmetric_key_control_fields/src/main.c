/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

static void print_controls(const char *label,
			   const stsafea_symmetric_key_slot_provisioning_ctrl_fields_t *controls)
{
	printk("%s: change_right=%u put_key=%u plaintext=%u derived=%u "
	       "wrapped_anonymous=%u ECDHE_anonymous=%u wrapped_auth_key=0x%02X "
	       "ECDHE_auth_key=0x%02X\n",
	       label, controls->change_right, controls->put_key, controls->plaintext,
	       controls->derived, controls->wrapped_anonymous, controls->ECDHE_anonymous,
	       controls->wrapped_authentication_key, controls->ECDHE_authentication_key);
}

int main(void)
{
	stsafea_symmetric_key_slot_provisioning_ctrl_fields_t controls;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;

	stsephyr_sample_banner(
		"STSAFE-A120 symmetric-key provisioning-control example",
		"Reads the selected slot policy and only changes it when the destructive opt-in "
		"is enabled.");
	printk("Target symmetric-key slot: %d\n", CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT);
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_get_symmetric_key_slot_provisioning_ctrl_fields(
		handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, &controls);
	if (stsephyr_sample_status("stse_get_symmetric_key_slot_provisioning_ctrl_fields",
				   status) != 0) {
		goto out;
	}
	print_controls("Current controls", &controls);

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_SYMMETRIC_CONTROL_UPDATE)
	if (controls.change_right == 0U) {
		printk("FAIL: slot control fields are locked and cannot be changed\n");
		goto out;
	}
	printk("WARNING: opt-in enabled; the slot provisioning policy will be changed "
	       "persistently.\n");
	printk("Existing permissions and change_right are preserved; anonymous wrapped and "
	       "ECDHE provisioning are enabled.\n");
	controls.wrapped_anonymous = 1U;
	controls.ECDHE_anonymous = 1U;
	controls.wrapped_authentication_key = 0xFFU;
	controls.ECDHE_authentication_key = 0xFFU;
	status = stse_set_symmetric_key_slot_provisioning_ctrl_fields(
		handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, &controls);
	if (stsephyr_sample_status("stse_set_symmetric_key_slot_provisioning_ctrl_fields",
				   status) != 0) {
		goto out;
	}
	status = stse_get_symmetric_key_slot_provisioning_ctrl_fields(
		handler, CONFIG_SAMPLE_STSAFE_SYMMETRIC_KEY_SLOT, &controls);
	if (stsephyr_sample_status("read back symmetric-key provisioning controls", status) != 0) {
		goto out;
	}
	print_controls("Updated controls", &controls);
#else
	printk("SAFE MODE: no control fields were changed.\n");
	printk("The update variant is compile-tested only and excluded from hardware runs.\n");
#endif

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("04_symmetric_key_control_fields");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
