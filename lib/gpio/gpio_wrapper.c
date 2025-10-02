#include "gpio_wrapper.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpiow, CONFIG_GPIO_LOG_LEVEL);

int gpiow_init(struct gpiow* gw, const struct gpio_dt_spec* spec, gpiow_dir_t dir, gpio_flags_t flags)
{
    if (!gw || !spec) {
        return -EINVAL;
    }

    if (!gpio_is_ready_dt(spec)) {
        return -ENODEV;
    }

    gw->spec = spec;
    gw->dir = dir;

    int ret;
    if (dir == GPIOW_DIR_OUTPUT) {
        ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT | flags);
    }
    else {
        ret = gpio_pin_configure_dt(spec, GPIO_INPUT | flags);
    }

    return ret;
}

int gpiow_add_callback(struct gpiow* gw, gpiow_callback_t cb, gpio_flags_t edge)
{
    if (!gw || gw->dir != GPIOW_DIR_INPUT) {
        return -EINVAL;
    }

    int ret = gpio_pin_interrupt_configure_dt(gw->spec, edge);
    if (ret) {
        return ret;
    }

    gpio_init_callback(&gw->cb_data, (gpio_callback_handler_t)cb, BIT(gw->spec->pin));
    gpio_add_callback(gw->spec->port, &gw->cb_data);

    return 0;
}

int gpiow_set(struct gpiow* gw, int value)
{
    if (!gw || gw->dir != GPIOW_DIR_OUTPUT) {
        return -EINVAL;
    }
    return gpio_pin_set_dt(gw->spec, value);
}

int gpiow_get(struct gpiow* gw, int value)
{
    if (!gw || gw->dir != GPIOW_DIR_INPUT) {
        return -EINVAL;
    }
    return gpio_pin_get_dt(gw->spec);
}

int gpiow_toggle(struct gpiow* gw)
{
    if (!gw || gw->dir != GPIOW_DIR_OUTPUT) {
        return -EINVAL;
    }
    return gpio_pin_toggle_dt(gw->spec);
}
