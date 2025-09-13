/*
 * Copyright (c) 2021, Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SD_CARD_LIB_H_
#define SD_CARD_LIB_H_

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT   "/SD:"

int lsdir(const char* path);
int check_file_dir_exists(const char* path);
int sd_card_file_write(struct fs_file_t file, const char* file_path, char* text_str, size_t text_str_size);
int sd_card_file_read(struct fs_file_t file, const char* file_path, char* text_str, size_t text_str_size);

#endif /* SD_CARD_LIB_H_ */
