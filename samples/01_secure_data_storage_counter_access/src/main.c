/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <string.h>
#include <zephyr/sys/printk.h>

#define MAX_PARTITIONS	 16U
#define READ_BUFFER_SIZE 16U
#define READ_CHUNK_SIZE	 4U

int main(void)
{
	stsafea_data_partition_record_t partitions[MAX_PARTITIONS];
	uint8_t read_buffer[READ_BUFFER_SIZE];
	stsafea_data_partition_record_t *zone = NULL;
	stse_Handler_t *handler;
	stse_ReturnCode_t status;
	uint32_t counter;
	uint8_t partition_count;

	stsephyr_sample_banner(
		"STSAFE-A120 secure data storage counter access example",
		"Queries the data partition table, reads a counter zone and, when explicitly "
		"enabled, decrements its one-way counter while updating its associated data.");
	printk("Target counter zone: %02d (STSAFE-A120 SPL05 default personalization)\n",
	       CONFIG_SAMPLE_STSAFE_COUNTER_ZONE);
	printk("Counter decrements are permanent; adapt the zone and access parameters for other "
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
		if (partitions[i].index == CONFIG_SAMPLE_STSAFE_COUNTER_ZONE) {
			zone = &partitions[i];
		}
	}
	if ((zone == NULL) || (zone->zone_type != 1U)) {
		printk("FAIL: zone %d is not an available counter zone\n",
		       CONFIG_SAMPLE_STSAFE_COUNTER_ZONE);
		goto out;
	}
	if (zone->data_segment_length < sizeof(read_buffer)) {
		printk("FAIL: counter zone %u contains only %u associated-data bytes; "
		       "this example requires %u\n",
		       zone->index, zone->data_segment_length, (unsigned int)sizeof(read_buffer));
		goto out;
	}

	stsephyr_sample_section("Read the counter zone and its associated data");
	status = stse_data_storage_read_counter_zone(handler, zone->index, 0, read_buffer,
						     sizeof(read_buffer), READ_CHUNK_SIZE, &counter,
						     STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_read_counter_zone", status) != 0) {
		goto out;
	}
	printk(" - Counter-zone read (zone: %02u, length: %u)\n", zone->index,
	       (unsigned int)sizeof(read_buffer));
	stsephyr_sample_hex("Associated data", read_buffer, sizeof(read_buffer));
	printk("Counter value: %u\n", (unsigned int)counter);

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_COUNTER_DECREMENT)
	uint8_t new_associated_data[READ_BUFFER_SIZE];
	uint32_t new_counter;

	stsephyr_sample_section("Decrement the one-way counter and update associated data");
	printk("WARNING: opt-in is enabled; this operation permanently consumes one counter "
	       "unit.\n");
	if (counter == 0U) {
		printk("FAIL: counter zone %u has reached zero and cannot be decremented\n",
		       zone->index);
		goto out;
	}
	stsephyr_sample_random(new_associated_data, sizeof(new_associated_data));
	stsephyr_sample_hex("New associated data to write", new_associated_data,
			    sizeof(new_associated_data));
	status = stse_data_storage_decrement_counter_zone(
		handler, zone->index, 1U, 0, new_associated_data, sizeof(new_associated_data),
		&new_counter, STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_decrement_counter_zone", status) != 0) {
		goto out;
	}
	printk(" - Counter-zone decrement (zone: %02u, length: %u, amount: 1)\n", zone->index,
	       (unsigned int)sizeof(new_associated_data));
	stsephyr_sample_hex("New associated data", new_associated_data,
			    sizeof(new_associated_data));
	printk("New counter value: %u\n", (unsigned int)new_counter);

	stsephyr_sample_section("Read back the counter zone");
	status = stse_data_storage_read_counter_zone(handler, zone->index, 0, read_buffer,
						     sizeof(read_buffer), READ_CHUNK_SIZE, &counter,
						     STSE_NO_PROT);
	if (stsephyr_sample_status("stse_data_storage_read_counter_zone", status) != 0) {
		goto out;
	}
	stsephyr_sample_hex("Associated data read back", read_buffer, sizeof(read_buffer));
	printk("Counter value read back: %u\n", (unsigned int)counter);
	if ((counter != new_counter) ||
	    (memcmp(read_buffer, new_associated_data, sizeof(read_buffer)) != 0)) {
		printk("FAIL: counter-zone readback does not match the decrement response\n");
		goto out;
	}
	printk(" - Counter and associated-data readback verification: SUCCESS\n");
#else
	stsephyr_sample_section("Counter decrement (safe mode)");
	printk("SKIPPED: persistent counter decrement is disabled.\n");
	printk("Enable CONFIG_SAMPLE_STSAFE_ALLOW_COUNTER_DECREMENT only on a device "
	       "whose one-way counter may be consumed.\n");
#endif

	stsephyr_sample_close();
	stsephyr_sample_footer();
	stsephyr_sample_pass("01_secure_data_storage_counter_access");
	return 0;

out:
	stsephyr_sample_close();
	stsephyr_sample_footer();
	return 0;
}
