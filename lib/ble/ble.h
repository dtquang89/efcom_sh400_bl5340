// =============================
// File: include/ble.h
// Description: Simple BLE (NUS) module API for Zephyr on nRF
// =============================

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** User callback for received bytes over NUS. */
typedef void (*ble_rx_cb_t)(const uint8_t* data, uint16_t len);

/** Optional: connection state callback */
typedef void (*ble_conn_cb_t)(bool connected);

/** Initialize BLE stack + NUS and start fast advertising.
 * @param device_name If NULL, uses CONFIG_BT_DEVICE_NAME.
 * @return 0 on success, <0 on error.
 */
int ble_init(const char* device_name);

/** Start or restart advertising (e.g., after disconnect). */
int ble_start_advertising(void);

/** Send a buffer over the Nordic UART Service. */
int ble_send(const uint8_t* data, uint16_t len);

/** Register the receive callback (nullable). */
void ble_register_rx_callback(ble_rx_cb_t cb);

/** Register a connection-state callback (nullable). */
void ble_register_conn_callback(ble_conn_cb_t cb);

/** Returns true if a central is connected. */
bool ble_is_connected(void);

/** If connected, request a disconnect. */
int ble_disconnect(void);

#ifdef __cplusplus
}
#endif