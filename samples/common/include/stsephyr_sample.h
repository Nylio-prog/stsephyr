/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STSEPHYR_SAMPLE_H_
#define STSEPHYR_SAMPLE_H_

#include <stddef.h>
#include <stdint.h>

#include <stsephyr/stsafe_a120.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t stsephyr_st_root_ca[];
extern const size_t stsephyr_st_root_ca_size;

int stsephyr_sample_open(stse_Handler_t **handler);
void stsephyr_sample_close(void);
void stsephyr_sample_banner(const char *title, const char *description);
void stsephyr_sample_section(const char *title);
void stsephyr_sample_hex(const char *label, const uint8_t *data, size_t length);
void stsephyr_sample_partition_table(const stsafea_data_partition_record_t *partitions,
				     uint8_t partition_count);
void stsephyr_sample_random(uint8_t *data, size_t length);
int stsephyr_sample_status(const char *operation, stse_ReturnCode_t status);
void stsephyr_sample_footer(void);
void stsephyr_sample_pass(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* STSEPHYR_SAMPLE_H_ */
