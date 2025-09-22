/**
 * @file main.c
 * @brief PWM RGB LED sample for Zephyr.
 *
 * Demonstrates basic RGB LED color cycling using PWM wrapper API.
 *
 * Copyright (c) 2016 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dt_interfaces.h"
#include "pwm_wrapper.h"

LOG_MODULE_REGISTER(main);

/** @brief Sleep time between color changes (milliseconds). */
#define SLEEP_TIME_MS 1000

/** @brief PWM spec for red LED (from devicetree). */
static const struct pwm_dt_spec red_pwm = PWM_DT_SPEC_GET(PWM_RED_NODE);
/** @brief PWM spec for green LED (from devicetree). */
static const struct pwm_dt_spec green_pwm = PWM_DT_SPEC_GET(PWM_GREEN_NODE);
/** @brief PWM spec for blue LED (from devicetree). */
static const struct pwm_dt_spec blue_pwm = PWM_DT_SPEC_GET(PWM_BLUE_NODE);

/** @brief RGB LED context. */
static struct pwm_rgb rgb;

/**
 * @brief Main entry point for PWM RGB LED sample.
 *
 * Initializes RGB LED context and cycles through colors using PWM.
 *
 * @return 0 Always returns 0.
 */

int main(void)
{
    LOG_INF("Starting the application...");

    int ret = pwm_rgb_init(&rgb, red_pwm.dev, red_pwm.channel, green_pwm.channel, blue_pwm.channel, red_pwm.period);

    if (ret < 0) {
        LOG_ERR("RGB init failed");
        return 0;
    }

    while (1) {
        pwm_rgb_set_color(&rgb, 255, 0, 0); /* Red */
        k_sleep(K_SECONDS(1));
        pwm_rgb_set_color(&rgb, 0, 255, 0); /* Green */
        k_sleep(K_SECONDS(1));
        pwm_rgb_set_color(&rgb, 0, 0, 255); /* Blue */
        k_sleep(K_SECONDS(1));
        pwm_rgb_off(&rgb);
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
