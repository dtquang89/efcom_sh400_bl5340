/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include <uart_async_adapter.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/usb/usb_device.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include <bluetooth/services/nus.h>

#if CONFIG_DK_LIBRARY
    #include <dk_buttons_and_leds.h>
#else
    #include "gpio_wrapper.h"
#endif

#include <zephyr/settings/settings.h>

#include <stdio.h>
#include <string.h>

#include "dt_interfaces.h"

#include "analog_wrapper.h"
#include "pwm_wrapper.h"

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include "sdcard.h"

#include <zephyr/drivers/i2s.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY  7

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// #define RUN_STATUS_LED         LED0_NODE
#define RUN_LED_BLINK_INTERVAL 1000

// #define CON_STATUS_LED LED1_NODE

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define UART_BUF_SIZE           CONFIG_BT_NUS_UART_BUFFER_SIZE
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_WAIT_FOR_RX        CONFIG_BT_NUS_UART_RX_WAIT_TIME

#define I2C_NODE DT_NODELABEL(i2c1)

/** @brief MAX31341 RTC I2C address (7-bit). */
#define MAX31341_I2C_ADDR 0x69
/** @brief MAX31341 revision ID register address. */
#define MAX31341_REG_ID 0x59

/* GPIO variables definition */
static const struct gpio_dt_spec led0_spec = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1_spec = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2_spec = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static struct gpiow led0;
static struct gpiow led1;
static struct gpiow led2;

/* PWM variables definition */
/** @brief PWM spec for red LED (from devicetree). */
static const struct pwm_dt_spec red_pwm = PWM_DT_SPEC_GET(PWM_RED_NODE);
/** @brief PWM spec for green LED (from devicetree). */
static const struct pwm_dt_spec green_pwm = PWM_DT_SPEC_GET(PWM_GREEN_NODE);
/** @brief PWM spec for blue LED (from devicetree). */
static const struct pwm_dt_spec blue_pwm = PWM_DT_SPEC_GET(PWM_BLUE_NODE);

/** @brief RGB LED context. */
static struct pwm_rgb rgb;

/* ADC variables definition */
/* Build ADC channel spec directly from devicetree */
static struct adc_dt_spec adc_channel =
#if HAS_VOLTAGE_DIVIDER
    ADC_DT_SPEC_GET(VBATT_NODE)
#else
    ADC_SPEC
#endif
    ;

/* Analog wrapper context */
static struct analog_control_t adc_ctx;

/* SD Card */
#define TEST_FILE DISK_MOUNT_PT "/test.wav"

static FATFS fat_fs;
/** @brief Filesystem mount info. */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};
/** @brief File handle for test file. */
struct fs_file_t filep;
/** @brief SD card mount point. */
const char* disk_mount_pt = DISK_MOUNT_PT;

/* I2S */
#define NUMBER_OF_INIT_BUFFER 4
#define NUM_BLOCKS            20
#define BLOCK_SIZE            4 * 1024

enum player_state {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
};

struct audio_player
{
    struct fs_file_t* file;
    const struct device* i2s_dev;
    struct k_thread thread;
    struct k_sem cmd_sem;
    enum player_state state;
    bool stop_requested;
    void (*on_play_start)(void);
    void (*on_play_stop)(void);
    void (*on_play_end)(void);
};

static const struct device* dev_i2s = DEVICE_DT_GET(DT_NODELABEL(i2s_rxtx));
static struct audio_player player;

K_THREAD_STACK_DEFINE(player_stack, STACKSIZE);
K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

/* BLE UART variables definition */
static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct bt_conn* current_conn;
static struct bt_conn* auth_conn;
static struct k_work adv_work;

static const struct device* uart = DEVICE_DT_GET(DT_CHOSEN(nordic_nus_uart));
static struct k_work_delayable uart_work;

struct uart_data_t
{
    void* fifo_reserved;
    uint8_t data[UART_BUF_SIZE];
    uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

#ifdef CONFIG_UART_ASYNC_ADAPTER
UART_ASYNC_ADAPTER_INST_DEFINE(async_adapter);
#else
    #define async_adapter NULL
#endif

static void uart_cb(const struct device* dev, struct uart_event* evt, void* user_data)
{
    ARG_UNUSED(dev);

    static size_t aborted_len;
    struct uart_data_t* buf;
    static uint8_t* aborted_buf;
    static bool disable_req;

    switch (evt->type) {
    case UART_TX_DONE:
        LOG_DBG("UART_TX_DONE");
        if ((evt->data.tx.len == 0) || (!evt->data.tx.buf)) {
            return;
        }

        if (aborted_buf) {
            buf = CONTAINER_OF(aborted_buf, struct uart_data_t, data[0]);
            aborted_buf = NULL;
            aborted_len = 0;
        }
        else {
            buf = CONTAINER_OF(evt->data.tx.buf, struct uart_data_t, data[0]);
        }

        k_free(buf);

        buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
        if (!buf) {
            return;
        }

        if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS)) {
            LOG_WRN("Failed to send data over UART");
        }

        break;

    case UART_RX_RDY:
        LOG_DBG("UART_RX_RDY");
        buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data[0]);
        buf->len += evt->data.rx.len;

        if (disable_req) {
            return;
        }

        if ((evt->data.rx.buf[buf->len - 1] == '\n') || (evt->data.rx.buf[buf->len - 1] == '\r')) {
            disable_req = true;
            uart_rx_disable(uart);
        }

        break;

    case UART_RX_DISABLED:
        LOG_DBG("UART_RX_DISABLED");
        disable_req = false;

        buf = k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
        }
        else {
            LOG_WRN("Not able to allocate UART receive buffer");
            k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
            return;
        }

        uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);

        break;

    case UART_RX_BUF_REQUEST:
        LOG_DBG("UART_RX_BUF_REQUEST");
        buf = k_malloc(sizeof(*buf));
        if (buf) {
            buf->len = 0;
            uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
        }
        else {
            LOG_WRN("Not able to allocate UART receive buffer");
        }

        break;

    case UART_RX_BUF_RELEASED:
        LOG_DBG("UART_RX_BUF_RELEASED");
        buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t, data[0]);

        if (buf->len > 0) {
            k_fifo_put(&fifo_uart_rx_data, buf);
        }
        else {
            k_free(buf);
        }

        break;

    case UART_TX_ABORTED:
        LOG_DBG("UART_TX_ABORTED");
        if (!aborted_buf) {
            aborted_buf = (uint8_t*)evt->data.tx.buf;
        }

        aborted_len += evt->data.tx.len;
        buf = CONTAINER_OF((void*)aborted_buf, struct uart_data_t, data);

        uart_tx(uart, &buf->data[aborted_len], buf->len - aborted_len, SYS_FOREVER_MS);

        break;

    default:
        break;
    }
}

static void uart_work_handler(struct k_work* item)
{
    struct uart_data_t* buf;

    buf = k_malloc(sizeof(*buf));
    if (buf) {
        buf->len = 0;
    }
    else {
        LOG_WRN("Not able to allocate UART receive buffer");
        k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
        return;
    }

    uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);
}

static bool uart_test_async_api(const struct device* dev)
{
    const struct uart_driver_api* api = (const struct uart_driver_api*)dev->api;

    return (api->callback_set != NULL);
}

static int uart_init(void)
{
    int err;
    int pos;
    struct uart_data_t* rx;
    struct uart_data_t* tx;

    if (!device_is_ready(uart)) {
        return -ENODEV;
    }

    if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
        err = usb_enable(NULL);
        if (err && (err != -EALREADY)) {
            LOG_ERR("Failed to enable USB");
            return err;
        }
    }

    rx = k_malloc(sizeof(*rx));
    if (rx) {
        rx->len = 0;
    }
    else {
        return -ENOMEM;
    }

    k_work_init_delayable(&uart_work, uart_work_handler);

    if (IS_ENABLED(CONFIG_UART_ASYNC_ADAPTER) && !uart_test_async_api(uart)) {
        /* Implement API adapter */
        uart_async_adapter_init(async_adapter, uart);
        uart = async_adapter;
    }

    err = uart_callback_set(uart, uart_cb, NULL);
    if (err) {
        k_free(rx);
        LOG_ERR("Cannot initialize UART callback");
        return err;
    }

    if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
        LOG_INF("Wait for DTR");
        while (true) {
            uint32_t dtr = 0;

            uart_line_ctrl_get(uart, UART_LINE_CTRL_DTR, &dtr);
            if (dtr) {
                break;
            }
            /* Give CPU resources to low priority threads. */
            k_sleep(K_MSEC(100));
        }
        LOG_INF("DTR set");
        err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DCD, 1);
        if (err) {
            LOG_WRN("Failed to set DCD, ret code %d", err);
        }
        err = uart_line_ctrl_set(uart, UART_LINE_CTRL_DSR, 1);
        if (err) {
            LOG_WRN("Failed to set DSR, ret code %d", err);
        }
    }

    tx = k_malloc(sizeof(*tx));

    if (tx) {
        pos = snprintf(tx->data, sizeof(tx->data), "Starting Nordic UART service sample\r\n");

        if ((pos < 0) || (pos >= sizeof(tx->data))) {
            k_free(rx);
            k_free(tx);
            LOG_ERR("snprintf returned %d", pos);
            return -ENOMEM;
        }

        tx->len = pos;
    }
    else {
        k_free(rx);
        return -ENOMEM;
    }

    err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
    if (err) {
        k_free(rx);
        k_free(tx);
        LOG_ERR("Cannot display welcome message (err: %d)", err);
        return err;
    }

    err = uart_rx_enable(uart, rx->data, sizeof(rx->data), UART_WAIT_FOR_RX);
    if (err) {
        LOG_ERR("Cannot enable uart reception (err: %d)", err);
        /* Free the rx buffer only because the tx buffer will be handled in the callback */
        k_free(rx);
    }

    return err;
}

static void adv_work_handler(struct k_work* work)
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising successfully started");
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void connected(struct bt_conn* conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (err) {
        LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected %s", addr);

    current_conn = bt_conn_ref(conn);
#if CONFIG_DK_LIBRARY
    dk_set_led_on(CON_STATUS_LED);
#else
    gpiow_set(&led1, 1);
#endif
}

static void disconnected(struct bt_conn* conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

    if (auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
#if CONFIG_DK_LIBRARY
        dk_set_led_off(CON_STATUS_LED);
#else
        gpiow_set(&led1, 0);
#endif
    }
}

static void recycled_cb(void)
{
    LOG_INF("Connection object available from previous conn. Disconnect is complete!");
    advertising_start();
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void security_changed(struct bt_conn* conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        LOG_INF("Security changed: %s level %u", addr, level);
    }
    else {
        LOG_WRN("Security failed: %s level %u err %d %s", addr, level, err, bt_security_err_to_str(err));
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycled_cb,
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_NUS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn* conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn* conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    auth_conn = bt_conn_ref(conn);

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);

    if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX) || IS_ENABLED(CONFIG_SOC_SERIES_NRF54LX)) {
        LOG_INF("Press Button 0 to confirm, Button 1 to reject.");
    }
    else {
        LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
    }
}

static void auth_cancel(struct bt_conn* conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn* conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing failed conn: %s, reason %d %s", addr, reason, bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {.pairing_complete = pairing_complete, .pairing_failed = pairing_failed};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

static void bt_receive_cb(struct bt_conn* conn, const uint8_t* const data, uint16_t len)
{
    int err;
    char addr[BT_ADDR_LE_STR_LEN] = {0};

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

    LOG_INF("Received data from: %s", addr);

    for (uint16_t pos = 0; pos != len;) {
        struct uart_data_t* tx = k_malloc(sizeof(*tx));

        if (!tx) {
            LOG_WRN("Not able to allocate UART send data buffer");
            return;
        }

        /* Keep the last byte of TX buffer for potential LF char. */
        size_t tx_data_size = sizeof(tx->data) - 1;

        if ((len - pos) > tx_data_size) {
            tx->len = tx_data_size;
        }
        else {
            tx->len = (len - pos);
        }

        memcpy(tx->data, &data[pos], tx->len);

        pos += tx->len;

        /* Append the LF character when the CR character triggered
         * transmission from the peer.
         */
        if ((pos == len) && (data[len - 1] == '\r')) {
            tx->data[tx->len] = '\n';
            tx->len++;
        }

        err = uart_tx(uart, tx->data, tx->len, SYS_FOREVER_MS);
        if (err) {
            k_fifo_put(&fifo_uart_tx_data, tx);
        }
    }
}

static struct bt_nus_cb nus_cb = {
    .received = bt_receive_cb,
};

void error(void)
{
#if CONFIG_DK_LIBRARY
    dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);
#endif

    while (true) {
        /* Spin for ever */
        k_sleep(K_MSEC(1000));
    }
}

#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
static void num_comp_reply(bool accept)
{
    if (accept) {
        bt_conn_auth_passkey_confirm(auth_conn);
        LOG_INF("Numeric Match, conn %p", (void*)auth_conn);
    }
    else {
        bt_conn_auth_cancel(auth_conn);
        LOG_INF("Numeric Reject, conn %p", (void*)auth_conn);
    }

    bt_conn_unref(auth_conn);
    auth_conn = NULL;
}

    #if CONFIG_DK_LIBRARY
void button_changed(uint32_t button_state, uint32_t has_changed)
{
    uint32_t buttons = button_state & has_changed;

    if (auth_conn) {
        if (buttons & KEY_PASSKEY_ACCEPT) {
            num_comp_reply(true);
        }

        if (buttons & KEY_PASSKEY_REJECT) {
            num_comp_reply(false);
        }
    }
}
    #endif
#endif /* CONFIG_BT_NUS_SECURITY_ENABLED */

static void configure_gpio(void)
{
    int err;

#if CONFIG_DK_LIBRARY
    #ifdef CONFIG_BT_NUS_SECURITY_ENABLED
    err = dk_buttons_init(button_changed);
    if (err) {
        LOG_ERR("Cannot init buttons (err: %d)", err);
    }
    #endif /* CONFIG_BT_NUS_SECURITY_ENABLED */

    err = dk_leds_init();
    if (err) {
        LOG_ERR("Cannot init LEDs (err: %d)", err);
    }
#else
    err = gpiow_init(&led0, &led0_spec, GPIOW_DIR_OUTPUT, GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL);
    if (err < 0) {
        LOG_ERR("Failed to init led0: %d", err);
    }

    err = gpiow_init(&led1, &led1_spec, GPIOW_DIR_OUTPUT, GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL);
    if (err < 0) {
        LOG_ERR("Failed to init led1: %d", err);
    }

    err = gpiow_init(&led2, &led2_spec, GPIOW_DIR_OUTPUT, GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL);
    if (err < 0) {
        LOG_ERR("Failed to init led2: %d", err);
    }
#endif
}

static void configure_pwm(void)
{
    int ret = pwm_rgb_init(&rgb, red_pwm.dev, red_pwm.channel, green_pwm.channel, blue_pwm.channel, red_pwm.period);

    if (ret < 0) {
        LOG_ERR("TEST FAILED: RGB init failed");
    }
}

static int i2s_init(void)
{
    struct i2s_config i2s_cfg;
    int ret;

    if (!device_is_ready(dev_i2s)) {
        LOG_ERR("I2S device not ready");
        return -ENODEV;
    }
    /* Configure I2S stream */
    i2s_cfg.word_size = 16U;
    i2s_cfg.channels = 2U;
    i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg.frame_clk_freq = 16000;
    i2s_cfg.block_size = BLOCK_SIZE;
    i2s_cfg.timeout = 2000;
    /* Configure the Transmit port as Master */
    i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg.mem_slab = &tx_0_mem_slab;
    ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S stream");
        return ret;
    }

    return 0;
}

static int play_sound_from_sd_card(const struct device* dev, struct fs_file_t* file)
{
    bool i2s_started = false;
    uint8_t the_number_of_init_buffer = 0;
    int err;

    // Playback loop
    while (1) {
        // Allocate memory for I2S TX
        void* tx_0_mem_block;

        err = k_mem_slab_alloc(&tx_0_mem_slab, &tx_0_mem_block, K_NO_WAIT);
        if (err) {
            LOG_ERR("Failed to allocate TX block: %d", err);
            break;
        }

        /* Read data from file*/
        int num_read_bytes = sd_card_file_read(file, tx_0_mem_block, BLOCK_SIZE);
        if (num_read_bytes < 0) {
            LOG_ERR("Error read file: error %d", err);
            break;
        }
        else if (num_read_bytes == 0) {
            LOG_INF("Reached end of file");
            k_mem_slab_free(&tx_0_mem_slab, tx_0_mem_block);
            break;
        }

        /* Playback data on I2S */
        LOG_INF("[PLAYING] Read bytes: %d", num_read_bytes);

        err = i2s_write(dev, tx_0_mem_block, BLOCK_SIZE);
        if (err) {
            k_mem_slab_free(&tx_0_mem_slab, tx_0_mem_block);
            LOG_ERR("Failed to write data: %d", err);
            break;
        }

        the_number_of_init_buffer++;
        if (the_number_of_init_buffer == NUMBER_OF_INIT_BUFFER && i2s_started == false) {
            // make sure to filled THE_NUMBER_OF_INIT_BUFFER before starting i2s
            LOG_INF("Start I2S: %d", the_number_of_init_buffer);
            i2s_started = true;
            err = i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_START);
            if (err < 0) {
                LOG_INF("Could not start I2S tx: %d", err);
                return err;
            }
        }
    }

    // Stop TX queue
    err = i2s_trigger(dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
    if (err < 0) {
        LOG_INF("Could not stop I2S tx: %d", err);
        return err;
    }
    LOG_INF("All I2S blocks written");

    return 0;
}

static int i2s_sdcard_test(void)
{
    /* raw disk i/o */
    do {
        static const char* disk_pdrv = DISK_DRIVE_NAME;
        uint64_t memory_size_mb;
        uint32_t block_count;
        uint32_t block_size;

        if (disk_access_init(disk_pdrv) != 0) {
            LOG_ERR("Storage init ERROR!");
            return -EPERM;
        }

        if (disk_access_status(disk_pdrv) != DISK_STATUS_OK) {
            LOG_ERR("Disk status not OK!");
            return -ENODEV;
        }

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
            LOG_ERR("Unable to get sector count");
            return -EINVAL;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
            LOG_ERR("Unable to get sector size");
            return -EINVAL;
        }
        LOG_INF("Sector size %u", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
    } while (0);

    mp.mnt_point = disk_mount_pt;

    int res = fs_mount(&mp);

    if (res == FR_OK) {
        LOG_INF("Disk mounted.");
        res = lsdir(disk_mount_pt);
        if (res != 0) {
            LOG_ERR("Error listing disk: err %d", res);
        }
    }
    else {
        LOG_ERR("Error mounting disk: error %d", res);
        return -ENXIO;
    }

    /* Init I2S */
    int err = i2s_init();

    if (err < 0) {
        LOG_ERR("I2S initialization failed");
        return err;
    }

    /* Open file from SD card */
    err = sd_card_file_open(&filep, TEST_FILE, 44);  // Skip WAV header, 44 bytes
    if (err != 0) {
        LOG_ERR("Error open file: error %d", err);
        return err;
    }

    /* Play the file to I2S */
    err = play_sound_from_sd_card(dev_i2s, &filep);
    if (err) {
        LOG_ERR("Error playing sound from SD card: %d", err);
    }

    /* Close file */
    sd_card_file_close(&filep);

    /* Unmound SD card */
    fs_unmount(&mp);
    LOG_INF("I2S+SDCARD: Test run ended!");

    return 0;
}

static int sdcard_init(void)
{
    /* raw disk i/o */
    do {
        static const char* disk_pdrv = DISK_DRIVE_NAME;
        uint64_t memory_size_mb;
        uint32_t block_count;
        uint32_t block_size;

        if (disk_access_init(disk_pdrv) != 0) {
            LOG_ERR("Storage init ERROR!");
            return -EPERM;
        }

        if (disk_access_status(disk_pdrv) != DISK_STATUS_OK) {
            LOG_ERR("Disk status not OK!");
            return -ENODEV;
        }

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
            LOG_ERR("Unable to get sector count");
            return -EINVAL;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
            LOG_ERR("Unable to get sector size");
            return -EINVAL;
        }
        LOG_INF("Sector size %u", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
    } while (0);

    mp.mnt_point = disk_mount_pt;

    int res = fs_mount(&mp);

    if (res == FR_OK) {
        LOG_INF("Disk mounted.");
        res = lsdir(disk_mount_pt);
        if (res != 0) {
            LOG_ERR("Error listing disk: err %d", res);
        }
    }
    else {
        LOG_ERR("Error mounting disk: error %d", res);
        return -ENXIO;
    }

    LOG_INF("SDCARD: Init successful!");

    return 0;
}

void player_thread_fn(void* p1, void* p2, void* p3)
{
    struct audio_player* ctx = p1;
    bool started = false;
    uint8_t buf_count = 0;
    int err;

    while (1) {
        k_sem_take(&ctx->cmd_sem, K_FOREVER);

        if (ctx->state != PLAYER_PLAYING || ctx->stop_requested) {
            continue;
        }

        void* tx_block;
        err = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
        if (err)
            continue;

        int bytes = sd_card_file_read(ctx->file, tx_block, BLOCK_SIZE);
        if (bytes <= 0) {
            LOG_INF("End of file reached");
            k_mem_slab_free(&tx_0_mem_slab, tx_block);
            if (ctx->on_play_end)
                ctx->on_play_end();
            ctx->state = PLAYER_STOPPED;
            i2s_trigger(ctx->i2s_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
            continue;
        }

        err = i2s_write(ctx->i2s_dev, tx_block, BLOCK_SIZE);
        if (err) {
            k_mem_slab_free(&tx_0_mem_slab, tx_block);
            continue;
        }

        if (++buf_count == NUMBER_OF_INIT_BUFFER && !started) {
            started = true;
            i2s_trigger(ctx->i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
            if (ctx->on_play_start)
                ctx->on_play_start();
        }

        if (ctx->stop_requested) {
            i2s_trigger(ctx->i2s_dev, I2S_DIR_TX, I2S_TRIGGER_STOP);
            if (ctx->on_play_stop)
                ctx->on_play_stop();
            ctx->state = PLAYER_STOPPED;
        }
    }
}

void player_init(const struct device* i2s_dev)
{
    player.i2s_dev = i2s_dev;
    player.state = PLAYER_STOPPED;
    k_sem_init(&player.cmd_sem, 0, 1);

    k_thread_create(&player.thread, player_stack, K_THREAD_STACK_SIZEOF(player_stack), player_thread_fn, &player, NULL, NULL, 5, 0,
                    K_NO_WAIT);
}

void player_set_callbacks(void (*on_start)(void), void (*on_stop)(void), void (*on_end)(void))
{
    player.on_play_start = on_start;
    player.on_play_stop = on_stop;
    player.on_play_end = on_end;
}

int player_play(struct fs_file_t* file)
{
    if (player.state == PLAYER_PLAYING)
        return -EBUSY;

    player.file = file;
    player.stop_requested = false;
    player.state = PLAYER_PLAYING;
    k_sem_give(&player.cmd_sem);
    return 0;
}

void player_stop(void)
{
    player.stop_requested = true;
    k_sem_give(&player.cmd_sem);
}

static void player_on_start_cb(void)
{
    LOG_INF("Playback started");
}

static void player_cb_on_stop_cb(void)
{
    LOG_INF("Playback stopped");
}

static void player_cb_on_end_cb(void)
{
    LOG_INF("Playback ended");
}

/**
 * @brief Pre-measurement callback for analog read.
 *
 * @param user_handle User context pointer.
 */
static void pre_battery_measurement_cb(void* user_handle)
{
    ARG_UNUSED(user_handle);
    /* Nothing to do */
}

/**
 * @brief Post-measurement callback for analog read.
 *
 * @param user_handle User context pointer.
 */
static void post_battery_measurement_cb(void* user_handle)
{
    ARG_UNUSED(user_handle);
    /* Nothing to do */
}

static int battery_test(void)
{
    int ret;
    int32_t batt_mv;
    int batt_pct;

#if !HAS_VOLTAGE_DIVIDER
    LOG_INF("No voltage divider configured, make sure the input voltage is within the ADC range!");
#else
    LOG_INF("Using voltage divider");
#endif

    /* Initialize wrapper */
    ret = analog_init(&adc_ctx, &adc_channel);
    if (ret) {
        LOG_ERR("ADC init failed (%d)", ret);
        return -1;
    }

    /* Register callbacks */
    analog_callbacks_t cbs = {
        .pre_measurement = pre_battery_measurement_cb,
        .post_measurement = post_battery_measurement_cb,
    };
    analog_register_callbacks(&adc_ctx, &cbs, NULL);

    ret = analog_read_battery_mv(&adc_ctx, &batt_mv);
    if (ret) {
        LOG_ERR("Failed to read battery voltage (%d)", ret);
        return -2;
    }
    else {
        batt_pct = analog_get_battery_level(&adc_ctx, 1100, 3300);
        LOG_INF("Battery: %d mV (%d%%)", batt_mv, batt_pct);
    }

    /* Not reached, but if you ever stop: */
    analog_deinit(&adc_ctx);

    return 0;
}

static int gpio_led_test(void)
{
    int err = 0;

    err += gpiow_set(&led0, 1);
    err += gpiow_set(&led1, 1);
    err += gpiow_set(&led2, 1);
    k_sleep(K_SECONDS(1));
    err += gpiow_set(&led0, 0);
    err += gpiow_set(&led1, 0);
    err += gpiow_set(&led2, 0);
    k_sleep(K_SECONDS(1));

    return err;
}

static int pwm_rgb_test(void)
{
    int err = pwm_rgb_set_color(&rgb, 255, 0, 0); /* Red */
    k_sleep(K_SECONDS(1));
    err += pwm_rgb_set_color(&rgb, 0, 255, 0); /* Green */
    k_sleep(K_SECONDS(1));
    err += pwm_rgb_set_color(&rgb, 0, 0, 255); /* Blue */
    k_sleep(K_SECONDS(1));
    err += pwm_rgb_off(&rgb);
    k_sleep(K_SECONDS(1));

    return err;
}

static int i2c_read_reg(const struct device* dev, uint8_t reg_addr, uint8_t* data)
{
    int ret;

    ret = i2c_write_read(dev, MAX31341_I2C_ADDR, &reg_addr, 1, data, 1);
    if (ret != 0) {
        LOG_ERR("Failed to read register 0x%02X: %d", reg_addr, ret);
        return ret;
    }

    return 0;
}

static int i2c_max31341_read_device_id_test()
{
    int ret;
    uint8_t reg_val;

    /* Verify RTC device is ready */
    const struct device* rtc_dev = DEVICE_DT_GET(I2C_NODE);
    if (!device_is_ready(rtc_dev)) {
        LOG_WRN("RTC device not ready, proceeding with I2C access");
    }

    LOG_INF("I2C device i2c1 ready");

    /* I2C is already configured via device tree, no need to reconfigure */
    LOG_INF("Using I2C address 0x%02X for MAX31341 RTC", MAX31341_I2C_ADDR);

    /* Read ID registers */
    ret = i2c_read_reg(rtc_dev, MAX31341_REG_ID, &reg_val);
    if (ret != 0) {
        LOG_ERR("Failed to read ID register");
        return ret;
    }

    LOG_INF("Device ID: 0x%02X", reg_val);

    return 0;
}

int main(void)
{
    int blink_status = 0;
    int err = 0;

    // Configuration peripherals
    configure_gpio();
    configure_pwm();

    // Main peripheral tests
    err = pwm_rgb_test();
    if (err < 0) {
        LOG_ERR("TEST FAILED: PWM test failed");
    }

    err = gpio_led_test();
    if (err < 0) {
        LOG_ERR("TEST FAILED: GPIO test failed");
    }

    err = battery_test();
    if (err < 0) {
        LOG_ERR("TEST FAILED: Battery test failed");
    }

    err = i2c_max31341_read_device_id_test();
    if (err < 0) {
        LOG_ERR("TEST FAILED: I2C test failed");
    }

    // Original I2S+SDCARD test (blocking mode)
    // err = i2s_sdcard_test();
    // if (err < 0) {
    //     LOG_ERR("TEST FAILED: I2S+SDCARD test failed");
    // }

    /* Init SD Card*/
    err = sdcard_init();

    if (err < 0) {
        LOG_ERR("SDCard initialization failed");
        return err;
    }

    /* Init I2S */
    err = i2s_init();

    if (err < 0) {
        LOG_ERR("I2S initialization failed");
        return err;
    }

    /* Init I2S player */
    player_init(dev_i2s);
    // Register player callbacks
    player_set_callbacks(player_on_start_cb, player_cb_on_stop_cb, player_cb_on_end_cb);

    // Open a file from SD Card
    sd_card_file_open(&filep, TEST_FILE, 44);  // Skip WAV header, 44 bytes
    player_play(&filep);

    /* Init UART */
    err = uart_init();
    if (err) {
        error();
    }

    if (IS_ENABLED(CONFIG_BT_NUS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            LOG_ERR("Failed to register authorization callbacks. (err: %d)", err);
            return 0;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            LOG_ERR("Failed to register authorization info callbacks. (err: %d)", err);
            return 0;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        error();
    }

    LOG_INF("Bluetooth initialized");

    k_sem_give(&ble_init_ok);

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("Failed to initialize UART service (err: %d)", err);
        return 0;
    }

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    for (;;) {
#if CONFIG_DK_LIBRARY
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
#else
        gpiow_set(&led0, (++blink_status) % 2);
#endif
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}

void ble_write_thread(void)
{
    /* Don't go any further until BLE is initialized */
    k_sem_take(&ble_init_ok, K_FOREVER);
    struct uart_data_t nus_data = {
        .len = 0,
    };

    for (;;) {
        /* Wait indefinitely for data to be sent over bluetooth */
        struct uart_data_t* buf = k_fifo_get(&fifo_uart_rx_data, K_FOREVER);

        int plen = MIN(sizeof(nus_data.data) - nus_data.len, buf->len);
        int loc = 0;

        while (plen > 0) {
            memcpy(&nus_data.data[nus_data.len], &buf->data[loc], plen);
            nus_data.len += plen;
            loc += plen;

            if (nus_data.len >= sizeof(nus_data.data) || (nus_data.data[nus_data.len - 1] == '\n')
                || (nus_data.data[nus_data.len - 1] == '\r'))
            {
                if (bt_nus_send(NULL, nus_data.data, nus_data.len)) {
                    LOG_WRN("Failed to send data over BLE connection");
                }
                nus_data.len = 0;
            }

            plen = MIN(sizeof(nus_data.data), buf->len - loc);
        }

        k_free(buf);
    }
}

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
