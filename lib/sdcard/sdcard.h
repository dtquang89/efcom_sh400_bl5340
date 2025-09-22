
/**
 * @file sdcard.h
 * @brief SD card utility API for Zephyr-based projects.
 */

#ifndef SD_CARD_LIB_H_
#define SD_CARD_LIB_H_

#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/disk_access.h>

/** @brief Disk drive name for SD card. */
#define DISK_DRIVE_NAME "SD"
/** @brief Mount point for SD card. */
#define DISK_MOUNT_PT "/SD:"

/**
 * @brief List contents of a directory.
 *
 * @param path Path to the directory.
 * @return 0 on success, negative error code on failure.
 */
int lsdir(const char* path);

/**
 * @brief Check if a file or directory exists.
 *
 * @param path Path to the file or directory.
 * @return 1 if exists, 0 if not, negative error code on failure.
 */
int check_file_dir_exists(const char* path);

/**
 * @brief Write text to a file on the SD card.
 *
 * @param file File structure.
 * @param file_path Path to the file.
 * @param text_str Text string to write.
 * @param text_str_size Size of the text string.
 * @return 0 on success, negative error code on failure.
 */
int sd_card_file_write(struct fs_file_t file, const char* file_path, char* text_str, size_t text_str_size);

/**
 * @brief Read text from a file on the SD card.
 *
 * @param file File structure.
 * @param file_path Path to the file.
 * @param text_str Buffer to store read text.
 * @param text_str_size Size of the buffer.
 * @return Number of bytes read on success, negative error code on failure.
 */
int sd_card_file_read(struct fs_file_t file, const char* file_path, char* text_str, size_t text_str_size);

#endif
