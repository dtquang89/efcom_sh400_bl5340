# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#######################

set(project_base_dir ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf)

#######################

set(prj_overlay_dir ${project_base_dir}/overlays)

#######################

list(APPEND BOARD_ROOT ${project_base_dir}/)
list(APPEND DTS_ROOT ${project_base_dir}/)

#######################

list(APPEND OVERLAY_CONFIG
  ${prj_overlay_dir}/segger-rtt.conf
)

#######################

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdcard)

target_sources(app PRIVATE src/main.c)
