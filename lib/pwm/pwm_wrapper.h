/**
 * @file pwm_wrapper.h
 * @author Quang Duong
 * @brief PWM wrapper API for Zephyr-based projects.
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef PWM_WRAPPER_H_
#define PWM_WRAPPER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

/**
 * @brief RGB LED context structure.
 *
 * Stores PWM device, channels, and period configuration.
 */
struct pwm_rgb
{
    const struct device* dev; /**< PWM device */
    uint32_t channel_r;       /**< PWM channel for Red */
    uint32_t channel_g;       /**< PWM channel for Green */
    uint32_t channel_b;       /**< PWM channel for Blue */
    uint32_t period;          /**< PWM period in nanoseconds */
};

/**
 * @brief Initialize RGB LED context.
 *
 * @param ctx     Pointer to RGB LED context
 * @param dev     PWM device
 * @param ch_r    Red channel index
 * @param ch_g    Green channel index
 * @param ch_b    Blue channel index
 * @param period  PWM period in nanoseconds
 *
 * @retval 0 On success
 * @retval -EINVAL If arguments are invalid
 * @retval -ENODEV If device not ready
 */
int pwm_rgb_init(struct pwm_rgb* ctx, const struct device* dev, uint32_t ch_r, uint32_t ch_g, uint32_t ch_b, uint32_t period);

/**
 * @brief Set RGB LED color.
 *
 * @param ctx Pointer to RGB LED context
 * @param r   Red intensity (0–255)
 * @param g   Green intensity (0–255)
 * @param b   Blue intensity (0–255)
 *
 * @retval 0 On success
 * @retval <0 On error
 */
int pwm_rgb_set_color(struct pwm_rgb* ctx, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turn off the RGB LED.
 *
 * @param ctx Pointer to RGB LED context
 *
 * @retval 0 On success
 * @retval <0 On error
 */
int pwm_rgb_off(struct pwm_rgb* ctx);

/**
 * @brief Deinitialize RGB LED context.
 *
 * @param ctx Pointer to RGB LED context
 *
 * @retval 0 On success
 * @retval -EINVAL If ctx is NULL
 */
int pwm_rgb_deinit(struct pwm_rgb* ctx);

#endif  // PWM_WRAPPER_H_