/**
 * @file ble.h
 * @author Quang Duong
 * @brief BLE wrapper API for Zephyr-based projects.
 * @version 0.1
 * @date 2025-09-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief User callback for received bytes over NUS.
 *
 * @param data Pointer to received data.
 * @param len  Length of received data.
 */
typedef void (*ble_rx_cb_t)(const uint8_t* data, uint16_t len);

/**
 * @brief Connection state callback.
 *
 * @param connected True if connected, false otherwise.
 */
typedef void (*ble_conn_cb_t)(bool connected);

/**
 * @brief Initialize BLE stack + NUS and start fast advertising.
 *
 * @param device_name If NULL, uses CONFIG_BT_DEVICE_NAME.
 * @return 0 on success, <0 on error.
 */
int ble_init(const char* device_name);

/**
 * @brief Start or restart advertising (e.g., after disconnect).
 *
 * @return 0 on success, <0 on error.
 */
int ble_start_advertising(void);

/**
 * @brief Send a buffer over the Nordic UART Service.
 *
 * @param data Pointer to data buffer.
 * @param len  Length of data to send.
 * @return 0 on success, <0 on error.
 */
int ble_send(const uint8_t* data, uint16_t len);

/**
 * @brief Register the receive callback (nullable).
 *
 * @param cb Callback function to register.
 */
void ble_register_rx_callback(ble_rx_cb_t cb);

/**
 * @brief Register a connection-state callback (nullable).
 *
 * @param cb Callback function to register.
 */
void ble_register_conn_callback(ble_conn_cb_t cb);

/**
 * @brief Returns true if a central is connected.
 *
 * @return True if connected, false otherwise.
 */
bool ble_is_connected(void);

/**
 * @brief If connected, request a disconnect.
 *
 * @return 0 on success, <0 on error.
 */
int ble_disconnect(void);

#ifdef __cplusplus
}
#endif