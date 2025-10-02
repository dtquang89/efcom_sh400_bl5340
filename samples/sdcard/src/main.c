/**
 * @file main.c
 * @brief SD card and filesystem sample for Zephyr.
 *
 * Demonstrates mounting, listing, and writing to an SD card using the filesystem API and SDHC driver.
 *
 * @author Quang Duong
 * @date 13.09.2025
 */

#include <ff.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include "sdcard.h"

/*
 * Note: The fatfs library can only mount strings inside _VOLUME_STRS in ffconf.h
 */

#define TEST_FILE DISK_MOUNT_PT "/TEST01.TXT"

static FATFS fat_fs;
/** @brief Filesystem mount info. */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};
/** @brief File handle for test file. */
struct fs_file_t filep;
/** @brief Test string to write to file. */
char test_str[] = "7,8,9\n";
/** @brief SD card mount point. */
const char* disk_mount_pt = DISK_MOUNT_PT;

LOG_MODULE_REGISTER(main);

/**
 * @brief Main entry point for SD card sample.
 *
 * Initializes SD card, mounts filesystem, lists directory, and writes to a file in a loop.
 *
 * @return 0 Always returns 0.
 */
int main(void)
{
    /* raw disk i/o */
    do {
        static const char* disk_pdrv = DISK_DRIVE_NAME;
        uint64_t memory_size_mb;
        uint32_t block_count;
        uint32_t block_size;

        if (disk_access_init(disk_pdrv) != 0) {
            LOG_ERR("Storage init ERROR!");
            break;
        }

        if (disk_access_status(disk_pdrv) != DISK_STATUS_OK) {
            LOG_ERR("Disk status not OK!");
            break;
        }

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
            LOG_ERR("Unable to get sector count");
            break;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
            LOG_ERR("Unable to get sector size");
            break;
        }
        LOG_INF("Sector size %u", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
    } while (0);

    mp.mnt_point = disk_mount_pt;

    int res = fs_mount(&mp);

    if (res == FR_OK) {
        LOG_INF("Disk mounted.");
        res = lsdir(disk_mount_pt);
        if (res != 0) {
            LOG_ERR("Error listing disk: err %d", res);
        }
    }
    else {
        LOG_ERR("Error mounting disk: error %d", res);
    }

    while (1) {
        k_sleep(K_MSEC(2000));

        res = sd_card_file_write(&filep, TEST_FILE, test_str, strlen(test_str));
        if (res != 0) {
            LOG_ERR("Error write file: error %d", res);
            break;
        }
    }

    fs_unmount(&mp);
    LOG_INF("Test run ended!");
    return 0;
}