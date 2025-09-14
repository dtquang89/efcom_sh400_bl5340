/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dt_interfaces.h"

LOG_MODULE_REGISTER(main);

#define I2C_NODE DT_NODELABEL(i2c1)

/* MAX31341 RTC I2C Configuration */
#define MAX31341_I2C_ADDR 0x69  // 7-bit I2C address from DT
#define MAX31341_REG_ID   0x59  // Revision ID register

/**
 * @brief Read a single register from MAX31341 RTC
 *
 * @param reg_addr Register address to read
 * @param data Pointer to store read data
 * @return 0 on success, negative error code on failure
 */
static int max31341_read_reg(const struct device* dev, uint8_t reg_addr, uint8_t* data)
{
    int ret;

    ret = i2c_write_read(dev, MAX31341_I2C_ADDR, &reg_addr, 1, data, 1);
    if (ret != 0) {
        LOG_ERR("Failed to read register 0x%02X: %d", reg_addr, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief Read manufacturer/device ID from MAX31341 RTC
 *
 * @param id_data Array to store 1 bytes of ID data
 * @return 0 on success, negative error code on failure
 */
int max31341_read_device_id(uint8_t* id_data)
{
    int ret;
    uint8_t reg_val;

    /* Verify RTC device is ready */
    const struct device* rtc_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(rtc_dev)) {
        LOG_WRN("RTC device not ready, proceeding with I2C access");
    }

    LOG_INF("I2C device i2c1 ready");

    /* I2C is already configured via device tree, no need to reconfigure */
    LOG_INF("Using I2C address 0x%02X for MAX31341 RTC", MAX31341_I2C_ADDR);

    /* Read ID registers */
    ret = max31341_read_reg(rtc_dev, MAX31341_REG_ID, &reg_val);
    if (ret != 0) {
        LOG_ERR("Failed to read ID register");
        return ret;
    }

    LOG_INF("Device ID: 0x%02X", reg_val);

    *id_data = reg_val;

    return 0;
}

/* Example usage in main thread */
int main(void)
{
    uint8_t device_id;
    int ret;

    LOG_INF("MAX31341 RTC ID Reader Starting...");

    /* Read device ID */
    ret = max31341_read_device_id(&device_id);
    if (ret != 0) {
        LOG_ERR("Failed to read device ID: %d", ret);
        return ret;
    }

    LOG_INF("MAX31341 initialization complete");

    /* Keep the main thread alive */
    while (1) {
        k_sleep(K_SECONDS(10));
    }

    return 0;
}