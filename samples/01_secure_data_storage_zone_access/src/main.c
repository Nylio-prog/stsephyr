/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define MAX_PARTITIONS 16U
#define DATA_BUFFER_SIZE 32U
#define READ_CHUNK_SIZE 4U

int main(void)
{
	stsafea_data_partition_record_t partitions[MAX_PARTITIONS];
	uint8_t original[DATA_BUFFER_SIZE];
	stsafea_data_partition_record_t *zone = NULL;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint8_t partition_count;
	uint16_t access_length;

	printk("STSEphyr: secure data storage zone access\n");
	if (stsephyr_sample_open(&handler) != 0) {
		return 0;
	}

	status = stse_data_storage_get_total_partition_count(handler, &partition_count);
	if (stsephyr_sample_status("query partition count", status) != 0) {
		goto out;
	}
	if (partition_count > MAX_PARTITIONS) {
		printk("FAIL: device reports %u partitions; example supports %u\n",
		       partition_count, MAX_PARTITIONS);
		goto out;
	}
	status = stse_data_storage_get_partitioning_table(handler, partition_count,
							 partitions, sizeof(partitions));
	if (stsephyr_sample_status("query partition table", status) != 0) {
		goto out;
	}

	for (uint8_t i = 0; i < partition_count; ++i) {
		printk("Zone %u: %s, %u data bytes\n", partitions[i].index,
		       partitions[i].zone_type == 1U ? "counter" : "data",
		       partitions[i].data_segment_length);
		if (partitions[i].index == CONFIG_SAMPLE_STSAFE_DATA_ZONE) {
			zone = &partitions[i];
		}
	}
	if ((zone == NULL) || (zone->zone_type != 0U)) {
		printk("FAIL: zone %d is not an available data zone\n",
		       CONFIG_SAMPLE_STSAFE_DATA_ZONE);
		goto out;
	}

	access_length = MIN((uint16_t)sizeof(original), zone->data_segment_length);
	status = stse_data_storage_read_data_zone(handler, zone->index, 0, original,
						  access_length, READ_CHUNK_SIZE, STSE_NO_PROT);
	if (stsephyr_sample_status("read data zone", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Zone contents", original, access_length);

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_ZONE_UPDATE)
	uint8_t replacement[DATA_BUFFER_SIZE];
	uint8_t verification[DATA_BUFFER_SIZE];

	printk("WARNING: zone-update opt-in enabled; changing persistent NVM\n");
	stsephyr_sample_random(replacement, access_length);
	status = stse_data_storage_update_data_zone(handler, zone->index, 0,
						    replacement, access_length,
						    STSE_NON_ATOMIC_ACCESS, STSE_NO_PROT);
	if (stsephyr_sample_status("update data zone", status) != 0) {
		goto out;
	}
	status = stse_data_storage_read_data_zone(handler, zone->index, 0,
						  verification, access_length,
						  READ_CHUNK_SIZE, STSE_NO_PROT);
	if ((status != STSE_OK) ||
	    (memcmp(replacement, verification, access_length) != 0)) {
		printk("FAIL: updated data did not verify\n");
		goto out;
	}
	printk("OK: updated data verified\n");
#else
	printk("SAFE MODE: zone update is disabled\n");
#endif

	stsephyr_sample_close();
	stsephyr_sample_pass("01_secure_data_storage_zone_access");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
