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
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
    #error "Unsupported board: led0 devicetree node label is not defined"
#endif

#if !DT_NODE_HAS_STATUS(LED1_NODE, okay)
    #error "Unsupported board: led1 devicetree node label is not defined"
#endif

#if !DT_NODE_HAS_STATUS(LED2_NODE, okay)
    #error "Unsupported board: led2 devicetree node label is not defined"
#endif

/* Outputs */
#define CODEC_IRQ_NODE DT_ALIAS(codec_irq)

#if !DT_NODE_HAS_STATUS(CODEC_IRQ_NODE, okay)
    #error "Unsupported board: codec_irq devicetree node label is not defined"
#endif

#define DRC_IN2_NODE DT_ALIAS(drc_in_2)

#if !DT_NODE_HAS_STATUS(DRC_IN2_NODE, okay)
    #error "Unsupported board: drc_in_2 devicetree node label is not defined"
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

/* Sensor/Chip */
#define EXT_RTC_NODE DT_NODELABEL(extrtc0)

#if !DT_NODE_HAS_STATUS(EXT_RTC_NODE, okay)
    #error "Unsupported board: extrtc0 devicetree node label is not defined"
#endif

#endif /* FIRMWARE_APPLICATION_SRC_DT_INTERFACES_H_ */