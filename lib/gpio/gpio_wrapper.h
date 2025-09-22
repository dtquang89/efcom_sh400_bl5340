/**
 * @file gpio_wrapper.h
 * @author Quang Duong
 * @brief GPIO wrapper API for Zephyr-based projects.
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef GPIO_WRAPPER_H_
#define GPIO_WRAPPER_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/**
 * @brief Enumeration for GPIO direction
 */
typedef enum { GPIOW_DIR_INPUT, GPIOW_DIR_OUTPUT } gpiow_dir_t;

/**
 * @brief GPIO callback type
 *
 * @param dev      GPIO device
 * @param cb       GPIO callback structure
 * @param pins     Bitmask of pins that triggered the callback
 */
typedef void (*gpiow_callback_t)(const struct device* dev, struct gpio_callback* cb, uint32_t pins);

/**
 * @brief GPIO wrapper context.
 */
struct gpiow
{
    const struct gpio_dt_spec* spec;
    struct gpio_callback cb_data;
    gpiow_dir_t dir;
};

/**
 * @brief Initialize a GPIO pin.
 *
 * @param gw    Pointer to wrapper instance
 * @param spec  GPIO DT spec pointer
 * @param dir   Direction (input/output)
 * @param flags Extra GPIO flags (pull-up, etc.)
 * @return 0 on success, negative errno otherwise
 */
int gpiow_init(struct gpiow* gw, const struct gpio_dt_spec* spec, gpiow_dir_t dir, gpio_flags_t flags);

/**
 * @brief Add callback for input pin
 *
 * @param gw        Pointer to wrapper instance
 * @param cb        Callback function
 * @param edge      GPIO_INT_EDGE_RISING, GPIO_INT_EDGE_FALLING, or BOTH
 * @return 0 on success, negative errno otherwise
 */
int gpiow_add_callback(struct gpiow* gw, gpiow_callback_t cb, gpio_flags_t edge);

/**
 * @brief Write output pin.
 *
 * @param gw    Pointer to wrapper instance
 * @param value Value to write (0 or 1)
 * @return 0 on success, negative errno otherwise
 */
int gpiow_write(struct gpiow* gw, int value);

/**
 * @brief Toggle output pin.
 *
 * @param gw Pointer to wrapper instance
 * @return 0 on success, negative errno otherwise
 */
int gpiow_toggle(struct gpiow* gw);

#endif  // GPIO_WRAPPER_H_
