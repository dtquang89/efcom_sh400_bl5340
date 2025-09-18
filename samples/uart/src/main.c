/**
 * @file main.c
 * @brief UART async wrapper sample for Zephyr.
 *
 * Demonstrates loopback UART communication using the UART wrapper API.
 * Supports both async and IRQ-driven backends.
 *
 * Copyright (c) 2025 Quang Duong
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include "dt_interfaces.h"
#include "uart_wrapper.h"

/** @brief UART device from devicetree. */
static const struct device* const uart_dev = DEVICE_DT_GET(UART_NODE);

#if CONFIG_UART_ASYNC_API
    #define RX_CHUNK 64
static uint8_t rx_a[RX_CHUNK]; /**< RX buffer A for async API. */
static uint8_t rx_b[RX_CHUNK]; /**< RX buffer B for async API. */
#endif

#define RING_SZ 256
static uint8_t rx_ring_storage[RING_SZ]; /**< RX ring buffer storage. */

static struct uart_ctx uctx; /**< UART wrapper context. */

/**
 * @brief UART TX done callback.
 *
 * Called when UART transmission is complete.
 *
 * @param ctx UART context pointer.
 * @param user User context pointer.
 */
static void tx_done_cb(struct uart_ctx* ctx, void* user)
{
    ARG_UNUSED(ctx);
    ARG_UNUSED(user);
    LOG_INF("IRQ TX done");
}

/**
 * @brief Main entry point for UART loopback sample.
 *
 * Initializes UART wrapper, enables RX, and echoes received data.
 *
 * @return 0 Always returns 0.
 */
int main(void)
{
    LOG_INF("Starting Loopback UART example");

    int rc;

#if CONFIG_UART_ASYNC_API
    rc = uaw_init(&uctx, uart_dev, rx_a, rx_b, sizeof(rx_a), 100, NULL, tx_done_cb, NULL);
#else
    rc = uaw_init(&uctx, uart_dev, NULL, NULL, 0, 0, NULL, tx_done_cb, NULL);
#endif

    if (rc) {
        LOG_ERR("uaw_init rc=%d", rc);
        return rc;
    }

    uaw_rx_ring_init(&uctx, rx_ring_storage, sizeof(rx_ring_storage));

#if CONFIG_UART_ASYNC_API
    uaw_rx_enable(&uctx);
#endif

    while (1) {
        uint8_t buf[32];

        size_t got = uaw_rx_get(&uctx, buf, sizeof(buf));
        if (got) {
            LOG_INF("RX: %.*s", got, buf);
        }

        const char* msg = "Hello World!\r\n";
        uaw_write(&uctx, msg, strlen(msg));

        k_sleep(K_SECONDS(1));
    }
    return 0;
}
