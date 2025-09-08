#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "analog_wrapper.h"
#include "dt_interfaces.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* Build ADC channel spec directly from devicetree */
static struct adc_dt_spec adc_channel =
#if HAS_VOLTAGE_DIVIDER
    ADC_DT_SPEC_GET(VBATT_NODE)
#else
    ADC_SPEC
#endif
    ;

/* Analog wrapper context */
static struct analog_control_t adc_ctx;

/* --- Callback functions --- */
static void pre_measurement_cb(void* user_handle)
{
    LOG_INF("Pre-measurement callback (user=%p)", user_handle);
}

static void post_measurement_cb(void* user_handle)
{
    LOG_INF("Post-measurement callback (user=%p)", user_handle);
}

int main(void)
{
    LOG_INF("Starting Battery measurement sample");

    int ret;
    int32_t batt_mv;
    int batt_pct;

#if !HAS_VOLTAGE_DIVIDER
    LOG_INF("No voltage divider configured, make sure the input voltage is within the ADC range!");
#else
    LOG_INF("Using voltage divider");
#endif

    /* Initialize wrapper */
    ret = analog_init(&adc_ctx, &adc_channel);
    if (ret) {
        LOG_ERR("ADC init failed (%d)", ret);
        return 0;
    }

    /* Register callbacks */
    analog_callbacks_t cbs = {
        .pre_measurement = pre_measurement_cb,
        .post_measurement = post_measurement_cb,
    };
    analog_register_callbacks(&adc_ctx, &cbs, NULL);

    while (1) {
        ret = analog_read_battery_mv(&adc_ctx, &batt_mv);
        if (ret) {
            LOG_ERR("Failed to read battery voltage (%d)", ret);
        }
        else {
            batt_pct = analog_get_battery_level(&adc_ctx, 1100, 3300);
            LOG_INF("Battery: %d mV (%d%%)", batt_mv, batt_pct);
        }

        k_sleep(K_SECONDS(2));
    }

    /* Not reached, but if you ever stop: */
    analog_deinit(&adc_ctx);

    return 0;
}
