/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define MAX_PARTITIONS	 16U
#define DATA_BUFFER_SIZE 100U
#define READ_CHUNK_SIZE	 4U

int main(void)
{
	stsafea_data_partition_record_t partitions[MAX_PARTITIONS];
	uint8_t original[DATA_BUFFER_SIZE];
	stsafea_data_partition_record_t *zone = NULL;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint8_t partition_count;

	stsephyr_sample_banner("STSAFE-A120 secure data storage zone access example",
			       "Queries the data partition table, reads a data zone and, when "
			       "explicitly enabled, replaces 100 bytes with random data generated "
			       "by STSAFE-A120 and verifies the readback.");
	printk("Target data zone: %02d (STSAFE-A120 SPL05 default personalization)\n",
	       CONFIG_SAMPLE_STSAFE_DATA_ZONE);
	printk("Zone updates change persistent NVM; adapt the zone and access parameters for other "
	       "personalizations.\n");

	stsephyr_sample_section("Initialize the target STSAFE-A120");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	stsephyr_sample_section("Query the data partition configuration");
	status = stse_data_storage_get_total_partition_count(handler, &partition_count);
	if (stsephyr_sample_status("stse_data_storage_get_total_partition_count", status) != 0) {
		goto out;
	}
	printk(" - Total partition count: %u\n", partition_count);
	if (partition_count > MAX_PARTITIONS) {
		printk("FAIL: device reports %u partitions; example supports %u\n", partition_count,
		       MAX_PARTITIONS);
		goto out;
	}
	status = stse_data_storage_get_partitioning_table(
		handler, partition_count, partitions,
		(uint16_t)(partition_count * sizeof(partitions[0])));
	if (stsephyr_sample_status("stse_data_storage_get_partitioning_table", status) != 0) {
		goto out;
	}
	stsephyr_sample_partition_table(partitions, partition_count);

	for (uint8_t i = 0; i < partition_count; ++i) {
		if (partitions[i].index == CONFIG_SAMPLE_STSAFE_DATA_ZONE) {
			zone = &partitions[i];
		}
	}
	if ((zone == NULL) || (zone->zone_type != 0U)) {
		printk("FAIL: zone %d is not an available data zone\n",
		       CONFIG_SAMPLE_STSAFE_DATA_ZONE);
		goto out;
	}
	if (zone->data_segment_length < sizeof(original)) {
		printk("FAIL: data zone %u contains only %u bytes; this SDK-equivalent "
		       "example requires %u\n",
		       zone->index, zone->data_segment_length, (unsigned int)sizeof(original));
		goto out;
	}

	stsephyr_sample_section("Read the selected data zone");
	status = stse_data_storage_read_data_zone(handler, zone->index, 0, original,
						  sizeof(original), READ_CHUNK_SIZE, STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_read_data_zone", status) != 0) {
		goto out;
	}
	printk(" - Data-zone read (zone: %02u, length: %u)\n", zone->index,
	       (unsigned int)sizeof(original));
	stsephyr_sample_hex("Original zone contents", original, sizeof(original));

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_ZONE_UPDATE)
	uint8_t replacement[DATA_BUFFER_SIZE];
	uint8_t verification[DATA_BUFFER_SIZE];

	stsephyr_sample_section("Generate replacement data inside STSAFE-A120");
	printk("WARNING: zone-update opt-in is enabled; this operation changes persistent NVM.\n");
	status = stse_generate_random(handler, replacement, sizeof(replacement));
	if (stsephyr_sample_status("stse_generate_random", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Proposed replacement data", replacement, sizeof(replacement));

	stsephyr_sample_section("Update the selected data zone");
	status = stse_data_storage_update_data_zone(handler, zone->index, 0, replacement,
						    sizeof(replacement), STSE_NON_ATOMIC_ACCESS,
						    STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_update_data_zone", status) != 0) {
		goto out;
	}
	printk(" - Data-zone update (zone: %02u, length: %u, non-atomic)\n", zone->index,
	       (unsigned int)sizeof(replacement));
	stsephyr_sample_hex("Data written", replacement, sizeof(replacement));

	stsephyr_sample_section("Read back and verify the updated data");
	status = stse_data_storage_read_data_zone(handler, zone->index, 0, verification,
						  sizeof(verification), READ_CHUNK_SIZE,
						  STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_read_data_zone", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Data read back", verification, sizeof(verification));
	if (memcmp(replacement, verification, sizeof(replacement)) != 0) {
		printk("FAIL: updated data readback does not match the generated data\n");
		goto out;
	}
	printk(" - Updated data readback verification: SUCCESS\n");
#else
	stsephyr_sample_section("Data-zone update (safe mode)");
	printk("SKIPPED: persistent data-zone update is disabled.\n");
	printk("Enable CONFIG_SAMPLE_STSAFE_ALLOW_ZONE_UPDATE only when changing "
	       "the selected zone is intentional.\n");
#endif

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("01_secure_data_storage_zone_access");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
