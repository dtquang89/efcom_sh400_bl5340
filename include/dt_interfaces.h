/******************************************************************************
 * @file dt_interfaces.h
 * @author Quang Duong
 * @brief Encapsulation of Devicetree-defines
 *******************************************************************************/

#ifndef FIRMWARE_APPLICATION_SRC_DT_INTERFACES_H_
#define FIRMWARE_APPLICATION_SRC_DT_INTERFACES_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

/* LED Indicator */
#define INDICATOR_NODE_0 DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(INDICATOR_NODE_0, okay)
    #error "Unsupported board: led0 devicetree node label is not defined"
#endif

/* UART */
#define UART_NODE DT_CHOSEN(zephyr_shell_uart)

#if !DT_NODE_HAS_STATUS(UART_NODE, okay)
    #error "Unsupported board: uart devicetree node label is not defined"
#endif

/* Battery measurement related */
#define VOLTAGE_DIVIDER_NODE DT_PATH(voltage_divider)

#if !DT_NODE_HAS_STATUS(VOLTAGE_DIVIDER_NODE, okay)
    #error "Unsupported board: voltage_divider devicetree node label is not defined"
#endif

#endif /* FIRMWARE_APPLICATION_SRC_DT_INTERFACES_H_ */