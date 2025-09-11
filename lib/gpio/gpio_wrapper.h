#ifndef GPIO_WRAPPER_H_
#define GPIO_WRAPPER_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

typedef enum { GPIOW_DIR_INPUT, GPIOW_DIR_OUTPUT } gpiow_dir_t;

typedef void (*gpiow_callback_t)(const struct device* dev, struct gpio_callback* cb, uint32_t pins);

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
 * @param user_data Custom pointer passed to callback
 * @return 0 on success, negative errno otherwise
 */
int gpiow_add_callback(struct gpiow* gw, gpiow_callback_t cb, gpio_flags_t edge);

/**
 * @brief Write output pin
 */
int gpiow_write(struct gpiow* gw, int value);

/**
 * @brief Toggle output pin
 */
int gpiow_toggle(struct gpiow* gw);

#endif  // GPIO_WRAPPER_H_
