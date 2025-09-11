# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_BLE_WRAPPER ble)
add_subdirectory_ifdef(CONFIG_UART_WRAPPER uart)
add_subdirectory_ifdef(CONFIG_GPIO_WRAPPER gpio)
add_subdirectory_ifdef(CONFIG_ANALOG_WRAPPER adc)
add_subdirectory_ifdef(CONFIG_PWM_WRAPPER pwm)