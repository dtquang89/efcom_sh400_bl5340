#-------------------------------------------------------------------------------
# Main Application
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# check if debug build
set(is_debug_build FALSE)

if(CMAKE_BUILD_TYPE)
    # add debug overlay for main application
    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type_lower)

    if((${build_type_lower} STREQUAL "develop") OR(${build_type_lower} STREQUAL "debug"))
        set(is_debug_build TRUE)
    endif()
endif()

if(is_debug_build)
    message(STATUS "> building a DEBUG build (Segger RTT activated)")
    
    list(APPEND OVERLAY_CONFIG
    ${CMAKE_CURRENT_SOURCE_DIR}/../overlays/segger-rtt.conf
    )
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(app PRIVATE src/main.c)
