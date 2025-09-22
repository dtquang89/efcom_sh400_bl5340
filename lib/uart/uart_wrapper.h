/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart_wrapper.h
 * @brief UART wrapper API for Zephyr-based projects.
 */

#ifndef UART_WRAPPER_H_
#define UART_WRAPPER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART context structure.
 *
 * Holds state and configuration for UART operations.
 */
struct uart_ctx;

/**
 * @brief RX callback type for UART wrapper.
 *
 * Called when data is received.
 *
 * @param ctx UART context pointer.
 * @param data Pointer to received data.
 * @param len Length of received data.
 * @param user_data User-provided context.
 */
typedef void (*uaw_rx_cb_t)(struct uart_ctx* ctx, const uint8_t* data, size_t len, void* user_data);

/**
 * @brief TX done callback type for UART wrapper.
 *
 * Called when transmission is complete.
 *
 * @param ctx UART context pointer.
 * @param user_data User-provided context.
 */
typedef void (*uaw_tx_done_cb_t)(struct uart_ctx* ctx, void* user_data);

/**
 * @brief UART backend type.
 */
enum uaw_backend {
    UAW_BACKEND_NONE,  /**< No backend selected */
    UAW_BACKEND_ASYNC, /**< Asynchronous backend */
    UAW_BACKEND_IRQ    /**< IRQ-driven backend */
};

/**
 * @brief TX node structure for FIFO queue.
 *
 * @param fifo_reserved Reserved for FIFO internal use.
 * @param len Length of data.
 * @param data Flexible array member for data.
 */
struct tx_node
{
    void* fifo_reserved;
    size_t len;
    uint8_t data[];
};

/**
 * @brief UART context structure.
 *
 * Stores configuration, buffers, callbacks, and state for UART operations.
 */
struct uart_ctx
{
    const struct device* uart; /**< UART device pointer */
    enum uaw_backend backend;  /**< Backend type */

    uint8_t* rx_buf[2];      /**< Double-buffer for RX */
    size_t rx_buf_len;       /**< RX buffer length */
    volatile uint8_t rx_idx; /**< Current RX buffer index */
    uint32_t rx_timeout_us;  /**< RX timeout in microseconds */

    struct ring_buf rx_ring; /**< RX ring buffer */

    struct k_fifo tx_fifo;      /**< TX FIFO queue */
    struct tx_node* tx_pending; /**< Pending TX node */
    size_t tx_progress;         /**< TX progress (IRQ mode) */

    uaw_rx_cb_t rx_cb;           /**< RX callback */
    uaw_tx_done_cb_t tx_done_cb; /**< TX done callback */
    void* user_data;             /**< User context */

    struct k_spinlock lock; /**< Spinlock for thread safety */
};

/**
 * @brief Initialize UART context.
 *
 * @param ctx UART context pointer.
 * @param uart_dev UART device pointer.
 * @param rx_a First RX buffer.
 * @param rx_b Second RX buffer.
 * @param rx_buf_len RX buffer length.
 * @param rx_timeout_us RX timeout in microseconds.
 * @param rx_cb RX callback.
 * @param tx_done_cb TX done callback.
 * @param user_data User context.
 * @return 0 on success, negative error code on failure.
 */
int uaw_init(struct uart_ctx* ctx, const struct device* uart_dev, uint8_t* rx_a, uint8_t* rx_b, size_t rx_buf_len, uint32_t rx_timeout_us,
             uaw_rx_cb_t rx_cb, uaw_tx_done_cb_t tx_done_cb, void* user_data);

/**
 * @brief Enable UART RX.
 *
 * @param ctx UART context pointer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_rx_enable(struct uart_ctx* ctx);

/**
 * @brief Disable UART RX.
 *
 * @param ctx UART context pointer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_rx_disable(struct uart_ctx* ctx);

/**
 * @brief Deinitialize UART RX.
 *
 * @param ctx UART context pointer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_rx_deinit(struct uart_ctx* ctx);

/**
 * @brief Initialize RX ring buffer.
 *
 * @param ctx UART context pointer.
 * @param ring_storage Storage for ring buffer.
 * @param ring_size Size of ring buffer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_rx_ring_init(struct uart_ctx* ctx, uint8_t* ring_storage, size_t ring_size);

/**
 * @brief Get data from RX ring buffer.
 *
 * @param ctx UART context pointer.
 * @param dst Destination buffer.
 * @param len Number of bytes to read.
 * @return Number of bytes read.
 */
size_t uaw_rx_get(struct uart_ctx* ctx, uint8_t* dst, size_t len);

/**
 * @brief Write data to UART.
 *
 * @param ctx UART context pointer.
 * @param data Data to send.
 * @param len Length of data.
 * @return 0 on success, negative error code on failure.
 */
int uaw_write(struct uart_ctx* ctx, const void* data, size_t len);

/**
 * @brief Check if UART TX is busy.
 *
 * @param ctx UART context pointer.
 * @return true if busy, false otherwise.
 */
bool uaw_tx_busy(struct uart_ctx* ctx);

/**
 * @brief Cancel and flush UART TX.
 *
 * @param ctx UART context pointer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_tx_cancel_and_flush(struct uart_ctx* ctx);

/**
 * @brief Deinitialize UART context.
 *
 * @param ctx UART context pointer.
 * @return 0 on success, negative error code on failure.
 */
int uaw_deinit(struct uart_ctx* ctx);

/**
 * @brief Get UART device from context.
 *
 * @param ctx UART context pointer.
 * @return UART device pointer.
 */
static inline const struct device* uaw_device(struct uart_ctx* ctx)
{
    return ctx->uart;
}

#ifdef __cplusplus
}
#endif

#endif