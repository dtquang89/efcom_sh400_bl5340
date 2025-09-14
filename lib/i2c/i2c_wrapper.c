#include "i2c_wrapper.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2cw, CONFIG_I2C_LOG_LEVEL);

/* Internal i2c callback that raises the poll signal */
#if defined(CONFIG_I2C_CALLBACK)
static void i2cw_i2c_cb(const struct device* dev, int result, void* userdata)
{
    struct k_poll_signal* sig = (struct k_poll_signal*)userdata;
    if (sig) {
        /* raise the signal with the result; worker thread will pick it up */
        k_poll_signal_raise(sig, result);
    }
}
#endif

static void i2c_worker(void* p1, void* p2, void* p3)
{
    struct i2c_ctx* ctx = (struct i2c_ctx*)p1;

    while (ctx->worker_running) {
        k_poll(&ctx->async_event, 1, K_FOREVER);

        unsigned int signaled;
        int result;

        k_poll_signal_check(&ctx->async_signal, &signaled, &result);
        k_poll_signal_reset(&ctx->async_signal);

        if (signaled && ctx->callback) {
            ctx->callback(ctx->cb_user_data, result, ctx->rx_buf, ctx->rx_len);
        }
    }
}

int i2cw_init(struct i2c_ctx* ctx, const struct i2c_dt_spec* bus_dt, k_thread_stack_t* stack, size_t stack_size, int prio)
{
    if (!ctx || !bus_dt) {
        return -EINVAL;
    }

    if (!device_is_ready(bus_dt->bus)) {
        LOG_ERR("I2C bus device not ready");
        return -ENODEV;
    }

    ctx->bus = *bus_dt;
    k_poll_signal_init(&ctx->async_signal);
    ctx->async_event = (struct k_poll_event)K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &ctx->async_signal);

    ctx->callback = NULL;
    ctx->cb_user_data = NULL;
    ctx->rx_buf = NULL;
    ctx->rx_len = 0;

    ctx->worker_stack = stack;
    ctx->worker_stack_size = stack_size;
    ctx->worker_running = true;

    k_thread_create(&ctx->worker_thread, ctx->worker_stack, ctx->worker_stack_size, i2c_worker, ctx, NULL, NULL, prio, 0, K_NO_WAIT);

    return 0;
}

int i2cw_deinit(struct i2c_ctx* ctx)
{
    if (!ctx) {
        return -EINVAL;
    }

    /* Stop the worker thread loop */
    ctx->worker_running = false;

    /* Wake up worker in case it's blocked in k_poll() */
    k_poll_signal_raise(&ctx->async_signal, -ECANCELED);

    /* Wait until thread exits */
    k_thread_join(&ctx->worker_thread, K_FOREVER);

    ctx->callback = NULL;
    ctx->cb_user_data = NULL;
    ctx->rx_buf = NULL;
    ctx->rx_len = 0;

    return 0;
}

int i2cw_write(struct i2c_ctx* ctx, const uint8_t* buf, size_t len)
{
    if (!ctx || !buf || len == 0) {
        return -EINVAL;
    }

    return i2c_write_dt(&ctx->bus, buf, len);
}

int i2cw_read(struct i2c_ctx* ctx, uint8_t* buf, size_t len)
{
    if (!ctx || !buf || len == 0) {
        return -EINVAL;
    }

    return i2c_read_dt(&ctx->bus, buf, len);
}

int i2cw_write_read(struct i2c_ctx* ctx, const uint8_t* tx_buf, size_t tx_len, uint8_t* rx_buf, size_t rx_len)
{
    if (!ctx || !tx_buf || tx_len == 0 || !rx_buf || rx_len == 0) {
        return -EINVAL;
    }

    return i2c_write_read_dt(&ctx->bus, tx_buf, tx_len, rx_buf, rx_len);
}

/* ---- Async Support ---- */

int i2cw_register_callback(struct i2c_ctx* ctx, i2cw_callback_t cb, void* user_data)
{
    if (!ctx) {
        return -EINVAL;
    }
    ctx->callback = cb;
    ctx->cb_user_data = user_data;
    return 0;
}

int i2cw_async_write_read(struct i2c_ctx* ctx, const uint8_t* tx_buf, size_t tx_len, uint8_t* rx_buf, size_t rx_len)
{
    if (!ctx || !tx_buf || !rx_buf || tx_len == 0 || rx_len == 0) {
        return -EINVAL;
    }

    /* prepare (persistent) message descriptors stored in ctx */
    ctx->msgs[0].buf = (uint8_t*)tx_buf;
    ctx->msgs[0].len = tx_len;
    ctx->msgs[0].flags = I2C_MSG_WRITE;

    ctx->msgs[1].buf = rx_buf;
    ctx->msgs[1].len = rx_len;
    ctx->msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

    /* reset signal, store rx pointers for worker callback */
    k_poll_signal_reset(&ctx->async_signal);
    ctx->rx_buf = rx_buf;
    ctx->rx_len = rx_len;

    int ret = -ENOTSUP;

#if defined(CONFIG_POLL)
    /* Use signal variant (requires CONFIG_POLL) */
    ret = i2c_transfer_signal(ctx->bus.bus, ctx->msgs, 2, ctx->bus.addr, &ctx->async_signal);
#elif defined(CONFIG_I2C_CALLBACK)
    /* Use callback variant (requires CONFIG_I2C_CALLBACK).
     * We pass the k_poll_signal as userdata so our internal cb can raise it.
     */
    ret = i2c_transfer_cb(ctx->bus.bus, ctx->msgs, 2, ctx->bus.addr, (i2c_callback_t)i2cw_i2c_cb, &ctx->async_signal);
#else
    /* Neither CONFIG_POLL nor CONFIG_I2C_CALLBACK is enabled */
    ret = -ENOTSUP;
#endif

    if (ret) {
        LOG_ERR("Async transfer start failed (%d)", ret);
    }

    return ret;
}