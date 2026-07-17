# Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(stsephyr)

if(NOT DEFINED STSELIB_ROOT)
  if(DEFINED WEST_TOPDIR)
    set(STSELIB_ROOT "${WEST_TOPDIR}/modules/lib/stselib")
  else()
    message(FATAL_ERROR "Set STSELIB_ROOT to the STSELib v1.1.9 checkout")
  endif()
endif()

get_filename_component(STSELIB_ROOT "${STSELIB_ROOT}" ABSOLUTE)

if(NOT EXISTS "${STSELIB_ROOT}/stselib.h" OR
   NOT EXISTS "${STSELIB_ROOT}/core/stse_platform.h")
  message(FATAL_ERROR
    "STSELib was not found at ${STSELIB_ROOT}. Run west update or set STSELIB_ROOT.")
endif()

zephyr_include_directories(
  ${ZEPHYR_CURRENT_MODULE_DIR}/include
  ${ZEPHYR_CURRENT_MODULE_DIR}/platform
  ${STSELIB_ROOT}
)

zephyr_library_sources(
  stsafe_a120.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/platform/stse_platform_system.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/platform/stse_platform_i2c.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/platform/stse_platform_crc.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/platform/stse_platform_crypto.c

  ${STSELIB_ROOT}/api/stse_aes.c
  ${STSELIB_ROOT}/api/stse_asymmetric_keys_management.c
  ${STSELIB_ROOT}/api/stse_data_storage.c
  ${STSELIB_ROOT}/api/stse_derive_keys.c
  ${STSELIB_ROOT}/api/stse_device_authentication.c
  ${STSELIB_ROOT}/api/stse_device_management.c
  ${STSELIB_ROOT}/api/stse_ecc.c
  ${STSELIB_ROOT}/api/stse_hash.c
  ${STSELIB_ROOT}/api/stse_mac.c
  ${STSELIB_ROOT}/api/stse_random.c
  ${STSELIB_ROOT}/api/stse_symmetric_keys_management.c

  ${STSELIB_ROOT}/certificate/stse_certificate.c
  ${STSELIB_ROOT}/certificate/stse_certificate_crypto.c
  ${STSELIB_ROOT}/certificate/stse_certificate_prints.c
  ${STSELIB_ROOT}/certificate/stse_certificate_subparsing.c

  ${STSELIB_ROOT}/core/stse_device.c
  ${STSELIB_ROOT}/core/stse_frame.c
  ${STSELIB_ROOT}/core/stse_generic_typedef.c
  ${STSELIB_ROOT}/core/stse_platform.c
  ${STSELIB_ROOT}/core/stse_session.c

  ${STSELIB_ROOT}/services/stsafea/stsafea_aes.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_asymmetric_key_slots.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_commands.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_data_partition.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_derive_keys.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_ecc.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_echo.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_entity_auth.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_frame_transfer.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_hash.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_host_key_slot.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_low_power.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_mac.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_password.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_patch.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_public_key_slots.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_put_query.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_random.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_reset.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_sessions.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_symmetric_key_slots.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_timings.c
  ${STSELIB_ROOT}/services/stsafea/stsafea_wrap_unwrap.c
)
