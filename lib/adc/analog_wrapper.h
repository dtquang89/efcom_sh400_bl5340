/**
 * @file analog_wrapper.h
 * @brief Wrapper library around Zephyr ADC API for voltage and battery measurements.
 *
 * Provides simplified initialization, reading raw values,
 * converting to voltage, scaling with a voltage divider,
 * and estimating battery levels. Also supports registering
 * user callbacks before and after measurement steps.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Prototype for measurement step callback.
 *
 * Called before or after an ADC measurement step.
 *
 * @param user_handle Pointer to user-provided context
 */
typedef void (*analog_measurement_step_f)(void* user_handle);

/**
 * @brief Struct holding user callbacks.
 */
typedef struct analog_callbacks
{
    /** Called before the actual measurement */
    analog_measurement_step_f pre_measurement;

    /** Called after the actual measurement */
    analog_measurement_step_f post_measurement;
} analog_callbacks_t;

/**
 * @brief Control structure for ADC wrapper.
 *
 * Stores ADC configuration, cached values, and user callbacks.
 */
struct analog_control_t
{
    struct adc_dt_spec adc_dt;  ///< adc spec handle from device tree
    struct adc_sequence sequence_cfg;
    struct adc_sequence_options options;  ///< configuration of the ADC sequence options

    int32_t adc_value;       ///< last measured ADC value
    int32_t cached_voltage;  ///< last measured voltage

    double voltage_divider_scale;  ///< voltage divider scale factor
    uint8_t input_channel;         ///< original channel id. Cached to allow deactivation of ADC channel for power saving reasons.

    analog_callbacks_t cb_functions;  ///< callback functions
    void* cb_handle;                  ///< user defined handle passed to the callback functions
};

/**
 * @brief Initialize ADC wrapper from devicetree specification.
 *
 * @param ctx       Pointer to analog control context
 * @param adc_dt    ADC specification from devicetree
 *
 * @retval 0 On success
 * @retval -EINVAL Invalid arguments or configuration
 * @retval -ENODEV ADC device not ready
 * @retval <other> ADC driver error code
 */
int analog_init(struct analog_control_t* ctx, const struct adc_dt_spec* adc_dt);

/**
 * @brief Deinitialize ADC wrapper and reset state.
 *
 * @param ctx Pointer to analog control context
 *
 * @retval 0 On success
 * @retval -EINVAL If ctx is NULL
 */
int analog_deinit(struct analog_control_t* ctx);

/**
 * @brief Register callback functions for measurement steps.
 *
 * @param ctx       Pointer to analog control context
 * @param callbacks Pointer to callback structure (can be NULL to clear)
 * @param user_handle User-defined context passed to callbacks
 *
 * @retval 0 On success
 * @retval -EINVAL If ctx is NULL
 */
int analog_register_callbacks(struct analog_control_t* ctx, const analog_callbacks_t* callbacks, void* user_handle);

/**
 * @brief Read raw ADC value.
 *
 * Executes a synchronous ADC read and returns the raw sample value.
 *
 * @param ctx     Pointer to analog control context
 * @param raw_val Pointer to store raw ADC value
 *
 * @retval 0 On success
 * @retval -EINVAL Invalid arguments
 * @retval <other> ADC driver error code
 */
int analog_read_raw(struct analog_control_t* ctx, int16_t* raw_val);

/**
 * @brief Read voltage at ADC pin (mV, after divider).
 *
 * Converts raw ADC value into millivolts at the ADC input pin.
 *
 * @param ctx        Pointer to analog control context
 * @param voltage_mv Pointer to store voltage in millivolts
 *
 * @retval 0 On success
 * @retval <other> Error code from analog_read_raw()
 */
int analog_read_voltage_mv(struct analog_control_t* ctx, int32_t* voltage_mv);

/**
 * @brief Read battery voltage in mV (corrected for divider).
 *
 * Applies configured voltage divider scaling to reconstruct
 * the original battery voltage.
 *
 * @param ctx        Pointer to analog control context
 * @param battery_mv Pointer to store battery voltage in millivolts
 *
 * @retval 0 On success
 * @retval <other> Error code from analog_read_voltage_mv()
 */
int analog_read_battery_mv(struct analog_control_t* ctx, int32_t* battery_mv);

/**
 * @brief Estimate battery level percentage.
 *
 * Maps measured battery voltage into a percentage based on
 * configured minimum and maximum voltage thresholds.
 *
 * @param ctx    Pointer to analog control context
 * @param min_mv Voltage considered as 0% (empty)
 * @param max_mv Voltage considered as 100% (full)
 *
 * @return Battery level percentage [0–100]
 * @retval <0 On error
 */
int analog_get_battery_level(struct analog_control_t* ctx, int32_t min_mv, int32_t max_mv);

#ifdef __cplusplus
}
#endif