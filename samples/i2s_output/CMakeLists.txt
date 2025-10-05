# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND OVERLAY_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/../../overlays/segger-rtt.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdcard)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
zephyr_include_directories(include)