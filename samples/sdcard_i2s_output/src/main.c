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
#include <zephyr/drivers/i2s.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/iterable_sections.h>
#include "sdcard.h"

/*
 * Note: The fatfs library can only mount strings inside _VOLUME_STRS in ffconf.h
 */

#define TEST_FILE DISK_MOUNT_PT "/test.wav"

static FATFS fat_fs;
/** @brief Filesystem mount info. */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};
/** @brief File handle for test file. */
struct fs_file_t filep;
/** @brief SD card mount point. */
const char* disk_mount_pt = DISK_MOUNT_PT;

/* I2S stuff */
#define NUMBER_OF_INIT_BUFFER 4
#define NUM_BLOCKS            20
#define BLOCK_SIZE            4 * 1024

static const struct device* dev_i2s = DEVICE_DT_GET(DT_NODELABEL(i2s_rxtx));

K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

LOG_MODULE_REGISTER(main);

static int i2s_init(void)
{
    struct i2s_config i2s_cfg;
    int ret;

    if (!device_is_ready(dev_i2s)) {
        LOG_ERR("I2S device not ready");
        return -ENODEV;
    }
    /* Configure I2S stream */
    i2s_cfg.word_size = 16U;
    i2s_cfg.channels = 2U;
    i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg.frame_clk_freq = 16000;
    i2s_cfg.block_size = BLOCK_SIZE;
    i2s_cfg.timeout = 2000;
    /* Configure the Transmit port as Master */
    i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg.mem_slab = &tx_0_mem_slab;
    ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S stream");
        return ret;
    }

    return 0;
}

static int play_sound_from_sd_card(const struct device* dev, struct fs_file_t* file)
{
    bool i2s_started = false;
    uint8_t the_number_of_init_buffer = 0;
    int err;

    // Playback loop
    while (1) {
        // Allocate memory for I2S TX
        void* tx_0_mem_block;

        err = k_mem_slab_alloc(&tx_0_mem_slab, &tx_0_mem_block, K_NO_WAIT);
        if (err) {
            LOG_ERR("Failed to allocate TX block: %d", err);
            break;
        }

        /* Read data from file*/
        int num_read_bytes = sd_card_file_read(file, tx_0_mem_block, BLOCK_SIZE);
        if (num_read_bytes < 0) {
            LOG_ERR("Error read file: error %d", err);
            break;
        }
        else if (num_read_bytes == 0) {
            LOG_INF("Reached end of file");
            k_mem_slab_free(&tx_0_mem_slab, tx_0_mem_block);
            break;
        }

        /* Playback data on I2S */
        LOG_INF("[PLAYING] Read bytes: %d", num_read_bytes);

        err = i2s_write(dev, tx_0_mem_block, BLOCK_SIZE);
        if (err) {
            k_mem_slab_free(&tx_0_mem_slab, tx_0_mem_block);
            LOG_ERR("Failed to write data: %d", err);
            break;
        }

        the_number_of_init_buffer++;
        if (the_number_of_init_buffer == NUMBER_OF_INIT_BUFFER && i2s_started == false) {
            // make sure to filled THE_NUMBER_OF_INIT_BUFFER before starting i2s
            LOG_INF("Start I2S: %d", the_number_of_init_buffer);
            i2s_started = true;
            err = i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_START);
            if (err < 0) {
                LOG_INF("Could not start I2S tx: %d", err);
                return err;
            }
        }
    }

    // Stop TX queue
    err = i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
    if (err < 0) {
        LOG_INF("Could not stop I2S tx: %d", err);
        return err;
    }
    LOG_INF("All I2S blocks written");

    return 0;
}

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

    /* Init I2S */
    int err = i2s_init();

    if (err < 0) {
        LOG_ERR("I2S initialization failed");
        return err;
    }

    /* Open file from SD card */
    err = sd_card_file_open(&filep, TEST_FILE, 44);  // Skip WAV header, 44 bytes
    if (err != 0) {
        LOG_ERR("Error open file: error %d", err);
        return err;
    }

    /* Play the file to I2S */
    err = play_sound_from_sd_card(dev_i2s, &filep);
    if (err) {
        LOG_ERR("Error playing sound from SD card: %d", err);
    }

    /* Close file */
    sd_card_file_close(&filep);

    /* Unmound SD card */
    fs_unmount(&mp);
    LOG_INF("Test run ended!");

    while (1) {
        k_sleep(K_MSEC(2000));
    }

    return 0;
}