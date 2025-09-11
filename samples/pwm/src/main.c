/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "dt_interfaces.h"
#include "pwm_wrapper.h"

LOG_MODULE_REGISTER(main);

/* Sleep time */
#define SLEEP_TIME_MS 1000

/* Example: button on DT node label "red-pwm-led" */
static const struct pwm_dt_spec red_pwm = PWM_DT_SPEC_GET(PWM_RED_NODE);
static const struct pwm_dt_spec green_pwm = PWM_DT_SPEC_GET(PWM_GREEN_NODE);
static const struct pwm_dt_spec blue_pwm = PWM_DT_SPEC_GET(PWM_BLUE_NODE);

static struct pwm_rgb rgb;

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
