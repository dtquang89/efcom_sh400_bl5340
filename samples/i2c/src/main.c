/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dt_interfaces.h"
#include "i2c_wrapper.h"

LOG_MODULE_REGISTER(main);

/* Sleep time */
#define SLEEP_TIME_MS 1000

static const struct i2c_dt_spec i2c_dev = I2C_DT_SPEC_GET(AMPLIFIER_NODE);
static struct i2c_ctx i2c_handle;

K_THREAD_STACK_DEFINE(i2c_worker_stack, 1024);

static void i2c_done_cb(void* user_data, int result, uint8_t* buf, size_t len)
{
    if (result == 0) {
        LOG_INF("Async I2C read OK: 0x%02X", buf[0]);
    }
    else {
        LOG_ERR("Async I2C read failed (%d)", result);
    }
}

int main(void)
{
    LOG_INF("Starting the application...");

    int ret = 0;
    ret = i2cw_init(&i2c_handle, &i2c_dev, i2c_worker_stack, sizeof(i2c_worker_stack), K_PRIO_PREEMPT(0));

    if (ret < 0) {
        LOG_ERR("Init failed");
        return 0;
    }

    LOG_INF("I2C wrapper initialized");

    ret = i2cw_register_callback(&i2c_handle, i2c_done_cb, NULL);

    if (ret < 0) {
        LOG_ERR("Register callback failed");
        return 0;
    }

    uint8_t reg = 0x00;
    static uint8_t rx_buf[1];

    ret = i2cw_async_write_read(&i2c_handle, &reg, 1, rx_buf, 1);

    if (ret < 0) {
        LOG_ERR("Init failed");
        return 0;
    }

    /* Cleanup */
    i2cw_deinit(&i2c_handle);
    LOG_INF("I2C wrapper deinitialized");

    while (1) {
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
