/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stsephyr_sample.h"

#include <zephyr/sys/printk.h>

#define KEY_SLOT 1U
#define USAGE_LIMIT 255U
#define KEY_TYPE STSE_ECC_KT_BP_P_512
#define EXAMPLE_NAME "01_key_pair_generation_BRAINPOOL_P512"

#define HASH_SIZE(size) ((size) - ((size) % 16U))

int main(void)
{
	stse_…5961 tokens truncated…	printk("FAIL: device reports %u partitions; example supports %u\n",
		       partition_count, MAX_PARTITIONS);
		goto out;
	}
	status = stse_data_storage_get_partitioning_table(handler, partition_count,
							 partitions, sizeof(partitions));
	if (stsephyr_sample_status("query partition table", status) != 0) {
		goto out;
	}

	for (uint8_t i = 0; i < partition_count; ++i) {
		printk("Zone %u: %s, %u data bytes, counter=%u\n",
		       partitions[i].index,
		       partitions[i].zone_type == 1U ? "counter" : "data",
		       partitions[i].data_segment_length,
		       partitions[i].counter_value);
		if (partitions[i].index == CONFIG_SAMPLE_STSAFE_COUNTER_ZONE) {
			zone = &partitions[i];
		}
	}
	if ((zone == NULL) || (zone->zone_type != 1U)) {
		printk("FAIL: zone %d is not an available counter zone\n",
		       CONFIG_SAMPLE_STSAFE_COUNTER_ZONE);
		goto out;
	}

	read_length = MIN((uint16_t)sizeof(read_buffer), zone->data_segment_length);
	status = stse_data_storage_read_counter_zone(handler, zone->index, 0,
						     read_buffer, read_length,
						     READ_CHUNK_SIZE, &counter, STSE_NO_PROT);
	if (stsephyr_sample_status("read counter zone", status) != 0) {
		goto out;
	}
	printk("Zone %u current counter: %u\n", zone->index, counter);
	if (read_length > 0U) {
		stsephyr_sample_hex("Associated data", read_buffer, read_length);
	}

#if defined(CONFIG_SAMPLE_STSAFE_ALLOW_COUNTER_DECREMENT)
	printk("WARNING: decrement opt-in enabled; consuming one counter unit\n");
	status = stse_data_storage_decrement_counter_zone(handler, zone->index, 1U,
							   0, NULL, 0, &counter, STSE_NO_PROT);
	if (stsephyr_sample_status("decrement one-way counter", status) != 0) {
		goto out;
	}
	printk("Zone %u new counter: %u\n", zone->index, counter);
#else
	printk("SAFE MODE: counter decrement is disabled\n");
#endif

	stsephyr_sample_close();
	stsephyr_sample_pass("01_secure_data_storage_counter_access");
	return 0;

out:
	stsephyr_sample_close();
	return 0;
}
