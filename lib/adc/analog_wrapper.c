#include "analog_wrapper.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "dt_interfaces.h"

LOG_MODULE_REGISTER(analog_wrp, CONFIG_ANALOG_WRAPPER_LOG_LEVEL);

int analog_init(struct analog_control_t* ctx, const struct adc_dt_spec* adc_dt)
{
    if ((NULL == ctx) || (NULL == adc_dt)) {
        LOG_ERR("ctx or adc_config: NULL");
        return false;
    }

    if (!adc_is_ready_dt(adc_dt)) {
        LOG_ERR("ADC device %s is not ready", adc_dt->dev->name);
        return -ENODEV;
    }

    ctx->adc_dt = *adc_dt;

#if CONFIG_ADC_CONFIGURABLE_INPUTS
    ctx->input_channel = ctx->adc_dt.channel_cfg.input_positive;
#else
    ctx->input_channel = ctx->adc_dt.channel_cfg.channel_id;
#endif

    // try to set setup to find out if it works
    int retval = adc_channel_setup_dt(adc_dt);

    if (0 != retval) {
        LOG_ERR("could not configure analog input |%d|", retval);
        return retval;
    }

    // --- configure SAADC sequence
    retval = adc_sequence_init_dt(adc_dt, &ctx->sequence_cfg);

    if (0 != retval) {
        LOG_ERR("could not init sequence |%d|", retval);
        return retval;
    }

    ctx->options.callback = NULL;
    ctx->options.user_data = NULL;
    ctx->sequence_cfg.options = &ctx->options;
    ctx->sequence_cfg.buffer = NULL;
    ctx->sequence_cfg.buffer_size = 0;
    ctx->sequence_cfg.calibrate = true;

    ctx->voltage_divider_scale = 1.0;

#if HAS_VOLTAGE_DIVIDER
    uint32_t output_ohm = DT_PROP(VBATT_NODE, output_ohms);
    uint32_t full_ohm = DT_PROP(VBATT_NODE, full_ohms);

    if (output_ohm != 0) {
        ctx->voltage_divider_scale = (double)full_ohm / output_ohm;
    }
#endif

    ctx->cached_voltage = 0;
    ctx->cb_functions.pre_measurement = NULL;
    ctx->cb_functions.post_measurement = NULL;
    ctx->cb_handle = NULL;

    return 0;
}

int analog_deinit(struct analog_control_t* ctx)
{
    if (!ctx)
        return -EINVAL;

    ctx->sequence_cfg.buffer = NULL;
    ctx->sequence_cfg.buffer_size = 0;
    ctx->cached_voltage = 0;
    ctx->cb_functions.pre_measurement = NULL;
    ctx->cb_functions.post_measurement = NULL;
    ctx->cb_handle = NULL;

    return 0;
}

int analog_register_callbacks(struct analog_control_t* ctx, const analog_callbacks_t* callbacks, void* user_handle)
{
    if (!ctx)
        return -EINVAL;

    if (callbacks) {
        ctx->cb_functions = *callbacks;
        ctx->cb_handle = user_handle;
    }
    else {
        ctx->cb_functions.pre_measurement = NULL;
        ctx->cb_functions.post_measurement = NULL;
        ctx->cb_handle = NULL;
    }

    return 0;
}

int analog_read_raw(struct analog_control_t* ctx, int16_t* raw_val)
{
    if (!ctx || !raw_val)
        return -EINVAL;

    if (ctx->cb_functions.pre_measurement) {
        ctx->cb_functions.pre_measurement(ctx->cb_handle);
    }

    int retval = 0;

#if CONFIG_ADC_CONFIGURABLE_INPUTS

    // --- if there are configurable inputs, try to deactivate the ADC input pin
    ctx->adc_dt.channel_cfg.input_positive = ctx->input_channel;
    retval = adc_channel_setup_dt(&ctx->adc_dt);

    if (0 != retval) {
        LOG_ERR("could not enable analog input");
    }

#endif

    ctx->options.callback = NULL;
    ctx->options.user_data = NULL;

    ctx->sequence_cfg.buffer = &ctx->adc_value;
    ctx->sequence_cfg.buffer_size = sizeof(ctx->adc_value);

    retval = adc_read(ctx->adc_dt.dev, &ctx->sequence_cfg);
    if (retval) {
        LOG_ERR("ADC read failed (%d)", retval);
        return retval;
    }

    ctx->sequence_cfg.calibrate = false;  // recalibrate after enabling ADC input

    LOG_INF("raw adc value: %d", ctx->adc_value);

    *raw_val = ctx->adc_value;

    if (ctx->cb_functions.post_measurement) {
        ctx->cb_functions.post_measurement(ctx->cb_handle);
    }

    return 0;
}

int analog_read_voltage_mv(struct analog_control_t* ctx, int32_t* voltage_mv)
{
    int16_t raw_adc;
    int ret = analog_read_raw(ctx, &raw_adc);
    if (ret)
        return ret;

    int32_t raw_val32 = raw_adc; /* promote to 32-bit */

    ctx->cached_voltage = adc_raw_to_millivolts_dt(&ctx->adc_dt, &raw_val32);
    *voltage_mv = ctx->cached_voltage;

    return 0;
}

int analog_read_battery_mv(struct analog_control_t* ctx, int32_t* battery_mv)
{
    int32_t v_adc_mv;
    int ret = analog_read_voltage_mv(ctx, &v_adc_mv);
    if (ret)
        return ret;

    /* Scale back using divider formula: Vbat = Vadc * (R1+R2) / R2 */
    *battery_mv = v_adc_mv * ctx->voltage_divider_scale;

    return 0;
}

int analog_get_battery_level(struct analog_control_t* ctx, int32_t min_mv, int32_t max_mv)
{
    int32_t batt_mv;
    int ret = analog_read_battery_mv(ctx, &batt_mv);
    if (ret)
        return ret;

    if (batt_mv <= min_mv)
        return 0;
    if (batt_mv >= max_mv)
        return 100;

    return (batt_mv - min_mv) * 100 / (max_mv - min_mv);
}