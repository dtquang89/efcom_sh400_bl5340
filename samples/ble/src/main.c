/** @file main.c
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include "ble.h"

#define BUFFER_SIZE CONFIG_BT_NUS_UART_BUFFER_SIZE

/**
 * @brief Buffer for BLE UART data.
 */
struct buffer_data_t
{
    uint8_t data[BUFFER_SIZE];
    uint16_t len;
};

/**
 * @brief Callback for received BLE data (NUS).
 *
 * Echoes received data back to BLE and logs it.
 *
 * @param data Pointer to received data.
 * @param len  Length of received data.
 */
static void ble_receive_cb(const uint8_t* data, uint16_t len)
{
    uint16_t pos = 0;
    while (pos < len) {
        struct buffer_data_t* tx = k_malloc(sizeof(*tx));
        if (!tx) {
            LOG_WRN("Buffer alloc fail");
            return;
        }
        size_t chunk = MIN(sizeof(tx->data) - 1, len - pos);
        memcpy(tx->data, &data[pos], chunk);
        tx->len = chunk;
        pos += chunk;
        /* Append LF if peer sent CR */
        if ((pos == len && data[len - 1] == '\r') && tx->len < sizeof(tx->data)) {
            tx->data[tx->len++] = '\n';
        }

        // Do whatever you want with the received data
        // Print the data to the log
        LOG_INF("Received data from BLE: %.*s, len=%d", tx->len, tx->data, tx->len);

        // For example, send back the data to BLE to create a echo example
        ble_send(tx->data, tx->len);

        k_free(tx);
    }
}

/**
 * @brief Main entry point for BLE UART sample.
 *
 * Initializes BLE, registers callbacks, and runs main loop.
 *
 * @return 0 Always returns 0.
 */
int main(void)
{
    LOG_INF("Starting Bluetooth Peripheral UART example");

    /* Bring up BLE and register callbacks */
    int err = ble_init(NULL);
    if (err) {
        LOG_ERR("BLE init failed: %d", err);
        return 0;
    }

    ble_register_rx_callback(ble_receive_cb);
    ble_register_conn_callback(NULL); /* or provide your own, can be used to handling different connection state, like LED indicator */

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
