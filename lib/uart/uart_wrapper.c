/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

#include "uart_wrapper.h"

LOG_MODULE_REGISTER(uart_wrp, CONFIG_UART_WRAPPER_LOG_LEVEL);

#if CONFIG_UART_ASYNC_API
static void uaw_uart_cb(const struct device* dev, struct uart_event* evt, void* user_data)
{
    struct uart_ctx* ctx = (struct uart_ctx*)user_data;
    struct tx_node* node;
    int rc;

    switch (evt->type) {
    case UART_TX_DONE:
        if (ctx->tx_pending) {
            k_free(ctx->tx_pending);
            ctx->tx_pending = NULL;
        }
        node = k_fifo_get(&ctx->tx_fifo, K_NO_WAIT);
        if (node) {
            rc = uart_tx(dev, node->data, node->len, SYS_FOREVER_US);
            if (rc) {
                LOG_ERR("uart_tx failed rc=%d", rc);
                k_free(node);
            }
            else {
                ctx->tx_pending = node;
            }
        }
        if (ctx->tx_done_cb) {
            ctx->tx_done_cb(ctx, ctx->user_data);
        }
        break;

    case UART_RX_BUF_REQUEST:
        rc = uart_rx_buf_rsp(dev, ctx->rx_buf[ctx->rx_idx], ctx->rx_buf_len);
        __ASSERT_NO_MSG(rc == 0);
        ctx->rx_idx ^= 1U;
        break;

    case UART_RX_RDY:
        if (evt->data.rx.len) {
            const uint8_t* ptr = evt->data.rx.buf + evt->data.rx.offset;
            if (ctx->rx_ring.size) {
                ring_buf_put(&ctx->rx_ring, ptr, evt->data.rx.len);
            }
            if (ctx->rx_cb) {
                ctx->rx_cb(ctx, ptr, evt->data.rx.len, ctx->user_data);
            }
        }
        break;

    default:
        break;
    }
}

static bool test_async_api(const struct device* dev)
{
    const struct uart_driver_api* api = (const struct uart_driver_api*)dev->api;
    return api->callback_set != NULL;
}
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN
static void uaw_irq_handler(const struct device* dev, void* user_data)
{
    struct uart_ctx* ctx = user_data;

    if (!uart_irq_update(dev)) {
        return;
    }

    if (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        /* RX ready */
        if (uart_irq_rx_ready(dev)) {
            uint8_t buf[32];
            int len = uart_fifo_read(dev, buf, sizeof(buf));
            if (len > 0) {
                ring_buf_put(&ctx->rx_ring, buf, len);
                if (ctx->rx_cb) {
                    ctx->rx_cb(ctx, buf, len, ctx->user_data);
                }
            }
        }

        /* TX ready */
        if (uart_irq_tx_ready(dev)) {
            if (ctx->tx_pending) {
                /* Keep filling until fifo can't accept more or buffer finished */
                while (ctx->tx_pending && uart_irq_tx_ready(dev)) {
                    size_t remaining = ctx->tx_pending->len - ctx->tx_progress;
                    if (remaining == 0) {
                        break;
                    }
                    int can_write = uart_irq_tx_ready(dev);
                    /* can_write returns >0: minimum bytes that may be written */
                    int to_write = (int)MIN(remaining, (size_t)can_write);
                    int written = uart_fifo_fill(dev, ctx->tx_pending->data + ctx->tx_progress, to_write);
                    if (written <= 0) {
                        /* hardware doesn't accept any now */
                        break;
                    }
                    ctx->tx_progress += written;
                }
                /* If we've copied all bytes into FIFO, keep TX IRQ enabled and wait
                 * for uart_irq_tx_complete() to become true later.
                 */
            }
            else {
                /* nothing to send */
                uart_irq_tx_disable(dev);
            }
        }

        /* TX complete */
        if (uart_irq_tx_complete(dev)) {
            if (ctx->tx_pending) {
                k_free(ctx->tx_pending);
                ctx->tx_pending = NULL;
                ctx->tx_progress = 0;
                if (ctx->tx_done_cb) {
                    ctx->tx_done_cb(ctx, ctx->user_data);
                }
            }
            /* Start next queued */
            struct tx_node* node = k_fifo_get(&ctx->tx_fifo, K_NO_WAIT);
            if (node) {
                ctx->tx_pending = node;
                ctx->tx_progress = 0;
                uart_irq_tx_enable(dev);
            }
            else {
                /* no more data: disable TX interrupts */
                uart_irq_tx_disable(dev);
            }
        }
    }
}
#endif

int uaw_init(struct uart_ctx* ctx, const struct device* uart_dev, uint8_t* rx_a, uint8_t* rx_b, size_t rx_buf_len, uint32_t rx_timeout_us,
             uaw_rx_cb_t rx_cb, uaw_tx_done_cb_t tx_done_cb, void* user_data)
{
    __ASSERT(ctx != NULL, "ctx required");
    __ASSERT(uart_dev != NULL, "uart_dev required");
    __ASSERT(rx_a != NULL && rx_b != NULL, "rx buffers required");
    __ASSERT(rx_buf_len > 0, "rx_buf_len must be > 0");

    memset(ctx, 0, sizeof(*ctx));
    ctx->uart = uart_dev;
    ctx->rx_buf[0] = rx_a;
    ctx->rx_buf[1] = rx_b;
    ctx->rx_buf_len = rx_buf_len;
    ctx->rx_idx = 1U;
    ctx->rx_timeout_us = rx_timeout_us;
    ctx->rx_cb = rx_cb;
    ctx->tx_done_cb = tx_done_cb;
    ctx->user_data = user_data;
    k_fifo_init(&ctx->tx_fifo);

    if (!device_is_ready(ctx->uart)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }

#if CONFIG_UART_ASYNC_API
    LOG_INF("Adding callback for ASYNC API");
    if (test_async_api(uart_dev)) {
        ctx->backend = UAW_BACKEND_ASYNC;
        return uart_callback_set(uart_dev, uaw_uart_cb, ctx);
    }
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN
    ctx->backend = UAW_BACKEND_IRQ;
    LOG_INF("Adding callback for IRQ backend");
    int ret = uart_irq_callback_user_data_set(uart_dev, uaw_irq_handler, ctx);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            LOG_ERR("Interrupt-driven UART API support not enabled");
        }
        else if (ret == -ENOSYS) {
            LOG_ERR("UART device does not support interrupt-driven API");
        }
        else {
            LOG_ERR("Error setting UART callback: %d", ret);
        }
        return -EIO;
    }

    uart_irq_rx_enable(uart_dev);
    return 0;
#endif
    return -ENOTSUP;
}

int uaw_rx_enable(struct uart_ctx* ctx)
{
    int rc = uart_rx_enable(ctx->uart, ctx->rx_buf[0], ctx->rx_buf_len, ctx->rx_timeout_us);
    if (rc && rc != -EALREADY) {
        LOG_ERR("uart_rx_enable failed rc=%d", rc);
    }
    return rc;
}

int uaw_rx_disable(struct uart_ctx* ctx)
{
    int rc = uart_rx_disable(ctx->uart);
    if (rc && rc != -EALREADY) {
        LOG_ERR("uart_rx_disable rc=%d", rc);
    }
    return rc;
}

int uaw_rx_deinit(struct uart_ctx* ctx)
{
    if (!ctx)
        return -EINVAL;
    (void)uart_rx_disable(ctx->uart);
    ring_buf_reset(&ctx->rx_ring);
    return 0;
}

int uaw_rx_ring_init(struct uart_ctx* ctx, uint8_t* ring_storage, size_t ring_size)
{
    if (!ctx || !ring_storage || !ring_size) {
        return -EINVAL;
    }
    ring_buf_init(&ctx->rx_ring, ring_size, ring_storage);
    return 0;
}

size_t uaw_rx_get(struct uart_ctx* ctx, uint8_t* dst, size_t len)
{
    if (!ctx || !dst || !len) {
        return 0;
    }
    return ring_buf_get(&ctx->rx_ring, dst, len);
}

bool uaw_tx_busy(struct uart_ctx* ctx)
{
    return ctx->tx_pending != NULL;
}

int uaw_tx_cancel_and_flush(struct uart_ctx* ctx)
{
    int rc = uart_tx_abort(ctx->uart);
    if (rc && rc != -EFAULT && rc != -ENOTSUP) {
        LOG_WRN("uart_tx_abort rc=%d", rc);
    }
    struct tx_node* n;
    while ((n = k_fifo_get(&ctx->tx_fifo, K_NO_WAIT)) != NULL) {
        k_free(n);
    }
    return 0;
}

int uaw_write(struct uart_ctx* ctx, const void* data, size_t len)
{
    if (!data || len == 0) {
        return -EINVAL;
    }

    struct tx_node* node = k_malloc(sizeof(struct tx_node) + len);
    if (!node) {
        return -ENOMEM;
    }
    node->len = len;
    memcpy(node->data, data, len);

    if (ctx->backend == UAW_BACKEND_ASYNC) {
        /* async path (your existing code) */
        unsigned int key = irq_lock();
        int start_immediately = (ctx->tx_pending == NULL);
        irq_unlock(key);

        if (start_immediately) {
            int rc = uart_tx(ctx->uart, node->data, node->len, SYS_FOREVER_US);
            if (rc == 0) {
                ctx->tx_pending = node;
                return 0;
            }
            if (rc != -EBUSY) {
                k_free(node);
                return rc;
            }
        }
        k_fifo_put(&ctx->tx_fifo, node);
        return 0;
    }

    if (ctx->backend == UAW_BACKEND_IRQ) {
        unsigned int key = irq_lock();
        if (!ctx->tx_pending) {
            ctx->tx_pending = node;
            ctx->tx_progress = 0;
            uart_irq_tx_enable(ctx->uart); /* kick off ISR to start filling */
            irq_unlock(key);
            return 0;
        }
        irq_unlock(key);
        k_fifo_put(&ctx->tx_fifo, node);
        return 0;
    }

    k_free(node);
    return -ENOTSUP;
}

int uaw_deinit(struct uart_ctx* ctx)
{
    if (!ctx)
        return -EINVAL;
    uaw_tx_cancel_and_flush(ctx);
    uaw_rx_deinit(ctx);
    (void)uart_callback_set(ctx->uart, NULL, NULL);
    return 0;
}
