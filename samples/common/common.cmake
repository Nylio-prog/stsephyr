# Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

target_sources(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/stsephyr_sample.c
  ${CMAKE_CURRENT_LIST_DIR}/src/st_root_ca.c
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
