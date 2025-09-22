/**
 * @file main.c
 * @brief GPIO sample application for Zephyr.
 *
 * Demonstrates basic GPIO input (button) and output (LED) usage with wrapper API.
 *
 * Copyright (c) 2016 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dt_interfaces.h"
#include "gpio_wrapper.h"

LOG_MODULE_REGISTER(main);

/* Sleep time */
#define SLEEP_TIME_MS 1000

/* Example: button on DT node label "drc_in2", LED on "led0" */
static const struct gpio_dt_spec button_spec = GPIO_DT_SPEC_GET(DRC_IN2_NODE, gpios);
static const struct gpio_dt_spec led_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct gpiow led;
static struct gpiow button;

/**
 * @brief Callback for button press event.
 *
 * Called when the button input pin triggers an interrupt.
 *
 * @param dev GPIO device pointer.
 * @param cb  GPIO callback structure pointer.
 * @param pins Bitmask of pins that triggered the callback.
 */
static void button_pressed_cb(const struct device* dev, struct gpio_callback* cb, uint32_t pins)
{
    LOG_INF("Button pressed!");
}

/**
 * @brief Main entry point for GPIO sample.
 *
 * Initializes GPIO pins, registers button callback, and toggles LED in a loop.
 *
 * @return 0 Always returns 0.
 */
int main(void)
{
    LOG_INF("Starting the application...");

    int ret = gpiow_init(&led, &led_spec, GPIOW_DIR_OUTPUT, GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL);
    if (ret < 0) {
        LOG_ERR("Failed to init led: %d", ret);
    }

    ret = gpiow_init(&button, &button_spec, GPIOW_DIR_INPUT, GPIO_PULL_UP);
    if (ret < 0) {
        LOG_ERR("Failed to configure button: %d", ret);
    }

    ret = gpiow_add_callback(&button, button_pressed_cb, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button: %d", ret);
    }

    LOG_INF("GPIO configuration done.");

    while (1) {
        gpiow_toggle(&led);
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
