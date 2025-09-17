/**
 * @file i2c_wrapper.h
 * @author Quang Duong
 * @brief I2C wrapper API for Zephyr-based projects.
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef I2C_WRAPPER_H_
#define I2C_WRAPPER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/**
 * @brief Async callback type.
 *
 * @param user_data  User context pointer
 * @param result     0 on success, <0 on error
 * @param buf        Buffer used in transfer (for reads, contains received data)
 * @param len        Number of bytes transferred
 */
typedef void (*i2cw_callback_t)(void* user_data, int result, uint8_t* buf, size_t len);

/**
 * @brief I2C wrapper context.
 */
struct i2c_ctx
{
    struct i2c_dt_spec bus;            /**< I2C bus specification (from DT) */
    struct k_poll_signal async_signal; /**< Poll signal for async ops */
    struct k_poll_event async_event;   /**< Poll event */
    i2cw_callback_t callback;          /**< Completion callback */
    void* cb_user_data;                /**< User context pointer */

    /* Async transfer state */
    uint8_t* rx_buf;
    size_t rx_len;

    /* Persistent messages buffer for async transfer (must live until transfer completes) */
    struct i2c_msg msgs[2];

    /* Worker thread */
    struct k_thread worker_thread;
    k_thread_stack_t* worker_stack;
    size_t worker_stack_size;
    bool worker_running;
};

/* Blocking API */
/**
 * @brief Initialize I2C wrapper from devicetree.
 *
 * @param ctx     Pointer to wrapper context
 * @param bus_dt  Devicetree I2C specification
 * @param stack   Pointer to thread stack memory
 * @param stack_size Size of thread stack
 * @param prio    Thread priority
 *
 * @retval 0 On success
 * @retval -ENODEV If device not ready
 * @retval -EINVAL If arguments invalid
 */
int i2cw_init(struct i2c_ctx* ctx, const struct i2c_dt_spec* bus_dt, k_thread_stack_t* stack, size_t stack_size, int prio);

/**
 * @brief Deinitialize I2C wrapper and stop worker thread.
 *
 * @param ctx Pointer to wrapper context
 *
 * @retval 0 On success
 * @retval -EINVAL If ctx is NULL
 */
int i2cw_deinit(struct i2c_ctx* ctx);

/**
 * @brief Write data to an I2C device.
 *
 * @param ctx   Pointer to wrapper context
 * @param buf   Buffer with data to send
 * @param len   Number of bytes to send
 *
 * @retval 0 On success
 * @retval <0 Error code
 */
int i2cw_write(struct i2c_ctx* ctx, const uint8_t* buf, size_t len);

/**
 * @brief Read data from an I2C device.
 *
 * @param ctx   Pointer to wrapper context
 * @param buf   Buffer to store read data
 * @param len   Number of bytes to read
 *
 * @retval 0 On success
 * @retval <0 Error code
 */
int i2cw_read(struct i2c_ctx* ctx, uint8_t* buf, size_t len);

/**
 * @brief Perform a write-read transaction (common for register reads).
 *
 * @param ctx      Pointer to wrapper context
 * @param tx_buf   Buffer with data to send
 * @param tx_len   Number of bytes to send
 * @param rx_buf   Buffer to store received data
 * @param rx_len   Number of bytes to read
 *
 * @retval 0 On success
 * @retval <0 Error code
 */
int i2cw_write_read(struct i2c_ctx* ctx, const uint8_t* tx_buf, size_t tx_len, uint8_t* rx_buf, size_t rx_len);

/* Async API */
/**
 * @brief Configure callback for async transfers.
 *
 * @param ctx        Wrapper context
 * @param cb         Callback function
 * @param user_data  Pointer passed back to callback
 */
int i2cw_register_callback(struct i2c_ctx* ctx, i2cw_callback_t cb, void* user_data);

/**
 * @brief Start asynchronous write-read transaction.
 *
 * @param ctx     Wrapper context
 * @param tx_buf  Buffer with data to send
 * @param tx_len  Number of bytes to send
 * @param rx_buf  Buffer to receive data
 * @param rx_len  Number of bytes to receive
 *
 * Callback is invoked when transfer completes.
 *
 * @retval 0 On success
 * @retval <0 Error code
 */
int i2cw_async_write_read(struct i2c_ctx* ctx, const uint8_t* tx_buf, size_t tx_len, uint8_t* rx_buf, size_t rx_len);

#endif  // I2C_WRAPPER_H_