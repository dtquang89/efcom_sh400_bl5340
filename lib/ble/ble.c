// =============================
// File: src/ble.c
// =============================
#include "ble.h"

#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_wrp, CONFIG_LOG_DEFAULT_LEVEL);

static struct bt_conn* curr_conn;
static struct bt_conn* auth_conn;
static struct k_work adv_work;
static ble_rx_cb_t s_rx_cb;
static ble_conn_cb_t s_conn_cb;
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static void bt_received_cb(struct bt_conn* conn, const uint8_t* const data, uint16_t len)
{
    if (s_rx_cb) {
        s_rx_cb(data, len);
    }
}

static struct bt_nus_cb nus_cb = {
    .received = bt_received_cb,
};

static void connected(struct bt_conn* conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (err) {
        LOG_ERR("BLE connect failed: 0x%02x", err);
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected %s", addr);

    curr_conn = bt_conn_ref(conn);

    if (s_conn_cb) {
        s_conn_cb(true);
    }
}

static void disconnected(struct bt_conn* conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s (reason %u)", addr, reason);

    if (auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    if (curr_conn) {
        bt_conn_unref(curr_conn);
        curr_conn = NULL;
    }

    if (s_conn_cb) {
        s_conn_cb(false);
    }
}

static void recycled_cb(void)
{
    LOG_DBG("Connection object recycled; ready to advertise.");
    (void)ble_start_advertising();
}

BT_CONN_CB_DEFINE(conn_cbs) = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled_cb,
};

static void adv_work_handler(struct k_work* work)
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising start failed (%d)", err);
    }
    else {
        LOG_INF("Advertising started");
    }
}

int ble_start_advertising(void)
{
    if (!k_work_is_pending(&adv_work)) {
        k_work_submit(&adv_work);
    }
    return 0;
}

bool ble_is_connected(void)
{
    return curr_conn != NULL;
}

int ble_disconnect(void)
{
    if (!curr_conn)
        return -ENOTCONN;
    return bt_conn_disconnect(curr_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

void ble_register_rx_callback(ble_rx_cb_t cb)
{
    s_rx_cb = cb;
}

void ble_register_conn_callback(ble_conn_cb_t cb)
{
    s_conn_cb = cb;
}

int ble_send(const uint8_t* data, uint16_t len)
{
    if (!data || !len)
        return -EINVAL;

    /* NULL as conn => send to all; Zephyr NUS supports NULL for the single active link */
    int err = bt_nus_send(NULL, data, len);
    if (err) {
        LOG_WRN("bt_nus_send failed: %d", err);
    }

    return err;
}

int ble_init(const char* device_name)
{
    ARG_UNUSED(device_name); /* If needed, set via settings or Kconfig */

    int err = 0;

    k_work_init(&adv_work, adv_work_handler);

    if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return err;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return err;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("bt_enable failed: %d", err);
        return err;
    }

    LOG_INF("Bluetooth enabled");

#if defined(CONFIG_SETTINGS)
    settings_load();
#endif

    err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("bt_nus_init failed: %d", err);
        return err;
    }

    return ble_start_advertising();
}
