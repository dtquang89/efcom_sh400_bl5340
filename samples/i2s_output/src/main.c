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
#include "test_wave.h"

/* I2S stuff */
#define NUMBER_OF_INIT_BUFFER 4
#define NUM_BLOCKS            8
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
    i2s_cfg.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
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

static int play_sound_from_memory(const uint8_t* wav_data, size_t wav_len)
{
    if (!wav_data || wav_len == 0) {
        return -1;
    }

    bool i2s_started = false;
    uint32_t number_of_init_buffer = 0;

    size_t offset = 0;
    while (offset < wav_len) {
        void* tx_0_mem_block;
        int error = k_mem_slab_alloc(&tx_0_mem_slab, &tx_0_mem_block, K_NO_WAIT);
        if (error != 0) {
            LOG_ERR("Failed to allocate TX block: %d", error);
            return error;
        }

        size_t to_copy = (wav_len - offset) < BLOCK_SIZE ? (wav_len - offset) : BLOCK_SIZE;
        /* If final block is short, zero-pad the remainder so i2s_write receives full block */
        memcpy(tx_0_mem_block, wav_data + offset, to_copy);
        if (to_copy < BLOCK_SIZE) {
            memset((uint8_t*)tx_0_mem_block + to_copy, 0, BLOCK_SIZE - to_copy);
        }

        offset += to_copy;

        error = i2s_write(dev_i2s, tx_0_mem_block, BLOCK_SIZE);
        if (error < 0) {
            k_mem_slab_free(&tx_0_mem_slab, tx_0_mem_block);
            LOG_ERR("Failed to write data: %d", error);
            return error;
        }

        number_of_init_buffer++;
        if (number_of_init_buffer == NUMBER_OF_INIT_BUFFER && i2s_started == false) {
            LOG_INF("Start I2S: %u", number_of_init_buffer);
            i2s_started = true;
            error = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
            if (error != 0) {
                LOG_INF("Could not start I2S tx: %d", error);
                return error;
            }
        }
    }

    /* Ensure stop and cleanup */
    int err = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
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
    /* Init I2S */
    int err = i2s_init();

    if (err < 0) {
        LOG_ERR("I2S initialization failed");
        return err;
    }

    /* Play the file to I2S */
    err = play_sound_from_memory(wav_mock_data + 44, wav_mock_data_len - 44);  // Skip WAV header, 44 bytes
    if (err) {
        LOG_ERR("Error playing sound from memory: %d", err);
    }

    LOG_INF("Test run ended!");

    while (1) {
        k_sleep(K_MSEC(2000));
    }

    return 0;
}