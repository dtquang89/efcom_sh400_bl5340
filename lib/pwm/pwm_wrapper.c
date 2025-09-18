#include "pwm_wrapper.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_rgb, CONFIG_PWM_LOG_LEVEL);

int pwm_rgb_init(struct pwm_rgb* ctx, const struct device* dev, uint32_t ch_r, uint32_t ch_g, uint32_t ch_b, uint32_t period)
{
    if (!ctx || !dev) {
        return -EINVAL;
    }
    if (!device_is_ready(dev)) {
        LOG_ERR("PWM device not ready");
        return -ENODEV;
    }

    ctx->dev = dev;
    ctx->channel_r = ch_r;
    ctx->channel_g = ch_g;
    ctx->channel_b = ch_b;
    ctx->period = period;

    return 0;
}

int pwm_rgb_set_color(struct pwm_rgb* ctx, uint8_t r, uint8_t g, uint8_t b)
{
    if (!ctx) {
        return -EINVAL;
    }

    /* Map 0–255 → 0–period (duty cycle in ns) */
    uint32_t duty_r = ((uint32_t)r * ctx->period) / 255;
    uint32_t duty_g = ((uint32_t)g * ctx->period) / 255;
    uint32_t duty_b = ((uint32_t)b * ctx->period) / 255;

    int ret = 0;
    ret |= pwm_set(ctx->dev, ctx->channel_r, ctx->period, duty_r, 0);
    ret |= pwm_set(ctx->dev, ctx->channel_g, ctx->period, duty_g, 0);
    ret |= pwm_set(ctx->dev, ctx->channel_b, ctx->period, duty_b, 0);

    if (ret) {
        LOG_ERR("PWM set failed (%d)", ret);
    }

    return ret;
}

int pwm_rgb_off(struct pwm_rgb* ctx)
{
    if (!ctx) {
        return -EINVAL;
    }
    int ret = 0;
    ret |= pwm_set(ctx->dev, ctx->channel_r, ctx->period, 0, 0);
    ret |= pwm_set(ctx->dev, ctx->channel_g, ctx->period, 0, 0);
    ret |= pwm_set(ctx->dev, ctx->channel_b, ctx->period, 0, 0);
    return ret;
}

int pwm_rgb_deinit(struct pwm_rgb* ctx)
{
    if (!ctx) {
        return -EINVAL;
    }
    pwm_rgb_off(ctx);
    ctx->dev = NULL;
    ctx->channel_r = ctx->channel_g = ctx->channel_b = 0;
    ctx->period = 0;
    return 0;
}
