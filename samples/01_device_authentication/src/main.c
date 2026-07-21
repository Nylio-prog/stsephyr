/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include "certificate/stse_certificate_prints.h"

#include <zephyr/sys/printk.h>

#define CERTIFICATE_ZONE	0U
#define PRIVATE_KEY_SLOT	0U
#define CERTIFICATE_BUFFER_SIZE 1024U

int main(void)
{
	uint8_t certificate[CERTIFICATE_BUFFER_SIZE];
	stse_certificate_t parsed_certificate;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint16_t certificate_size;

	stsephyr_sample_banner(
		"STSAFE-A120 Device Authentication Example",
		"Authenticate the secure element with its device certificate and private key.");
	printk("  The device certificate is shown before the one-call authentication so that\n");
	printk("  it can be inspected or forwarded to a remote authentication service.\n");

	stsephyr_sample_section("Initialize the target STSAFE-A120");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	stsephyr_sample_section("Read and inspect the device certificate");
	status = stse_get_device_certificate_size(handler, CERTIFICATE_ZONE, &certificate_size);
	if (stsephyr_sample_status("read device certificate size", status) != 0) {
		goto out;
	}
	if (certificate_size > sizeof(certificate)) {
		printk("FAIL: certificate needs %u bytes; buffer has %u\n", certificate_size,
		       (unsigned int)sizeof(certificate));
		goto out;
	}

	status = stse_get_device_certificate(handler, CERTIFICATE_ZONE, certificate_size,
					     certificate);
	if (stsephyr_sample_status("read device certificate", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Device certificate (DER)", certificate, certificate_size);

	status = stse_certificate_parse(certificate, &parsed_certificate, NULL);
	if (stsephyr_sample_status("parse device certificate", status) != 0) {
		goto out;
	}
	printk("\nParsed device certificate:");
	stse_certificate_print_parsed_cert(&parsed_certificate);
	printk("\n");

	stsephyr_sample_section("Authenticate the STSAFE-A120");
	printk("Certificate zone: %u; private-key slot: %u\n", CERTIFICATE_ZONE, PRIVATE_KEY_SLOT);
	status = stse_device_authenticate(handler, stsephyr_st_root_ca, CERTIFICATE_ZONE,
					  PRIVATE_KEY_SLOT);
	if (stsephyr_sample_status(
		    "device authentication over the STSE-A production CA certificate", status) !=
	    0) {
		goto out;
	}
	printk("Device certificate chain and proof of private-key possession verified.\n");

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("01_device_authentication");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
