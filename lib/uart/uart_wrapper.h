/*
 * SPDX-License-Identifier: Apache-2.0
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

struct uart_ctx;

typedef void (*uaw_rx_cb_t)(struct uart_ctx* ctx, const uint8_t* data, size_t len, void* user_data);

typedef void (*uaw_tx_done_cb_t)(struct uart_ctx* ctx, void* user_data);

enum uaw_backend { UAW_BACKEND_NONE, UAW_BACKEND_ASYNC, UAW_BACKEND_IRQ };

struct uart_ctx
{
    const struct device* uart;
    enum uaw_backend backend;

    uint8_t* rx_buf[2];
    size_t rx_buf_len;
    volatile uint8_t rx_idx;
    uint32_t rx_timeout_us;

    struct ring_buf rx_ring;

    struct k_fifo tx_fifo;
    struct tx_node* tx_pending;
    size_t tx_progress; /* used in IRQ mode */

    uaw_rx_cb_t rx_cb;
    uaw_tx_done_cb_t tx_done_cb;
    void* user_data;

    struct k_spinlock lock;
};

int uaw_init(struct uart_ctx* ctx, const struct device* uart_dev, uint8_t* rx_a, uint8_t* rx_b, size_t rx_buf_len, uint32_t rx_timeout_us,
             uaw_rx_cb_t rx_cb, uaw_tx_done_cb_t tx_done_cb, void* user_data);

int uaw_rx_enable(struct uart_ctx* ctx);
int uaw_rx_disable(struct uart_ctx* ctx);
int uaw_rx_deinit(struct uart_ctx* ctx);

int uaw_rx_ring_init(struct uart_ctx* ctx, uint8_t* ring_storage, size_t ring_size);
size_t uaw_rx_get(struct uart_ctx* ctx, uint8_t* dst, size_t len);

int uaw_write(struct uart_ctx* ctx, const void* data, size_t len);
bool uaw_tx_busy(struct uart_ctx* ctx);
int uaw_tx_cancel_and_flush(struct uart_ctx* ctx);

int uaw_deinit(struct uart_ctx* ctx);

static inline const struct device* uaw_device(struct uart_ctx* ctx)
{
    return ctx->uart;
}

#ifdef __cplusplus
}
#endif

#endif /* UART_WRAPPER_H_ */