/**
 * @file uart_comm.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief UART communication interface for host-device communication.
 * 
 * This module provides UART-based communication between the host computer and the
 * synchronization device. It implements a command protocol for controlling device
 * functions and receiving status information.
 * 
 * @version 2.3.0
 * @date 2024-09-05
 */

#pragma once

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <queue>          // priority queue, FIFO queue

#ifndef UNIT_TEST
#include <asf.h>
#include <string.h>
#include <strings.h>
#endif

#include "globals.h"

/**
 * @brief Data packet structure for serial communication protocol.
 * 
 * This structure defines the format of commands sent from the host to the device.
 * Each packet contains a command string and associated parameters for execution.
 */
typedef struct DataPacket
{
	char     cmd[4];    /**< 3-character command string, null-terminated */
	uint32_t arg1;      /**< First argument for the command */
	uint32_t arg2;      /**< Second argument for the command */
	uint32_t ts_us;     /**< Timestamp for command execution in microseconds */
	uint32_t N;         /**< Number of times to run command (0 = forever) */
	uint32_t interv_us; /**< Interval between command executions in microseconds */
} DataPacket;

/**
 * @brief Initialize UART communication interface.
 * 
 * Sets up UART peripheral, configures pins, enables interrupts, and initializes
 * DMA reception for continuous communication with the host.
 */
void init_uart_comm(void);

/**
 * @brief Send a null-terminated string to the host.
 * @param cstring Pointer to the null-terminated string to send
 * 
 * Sends the string using DMA for efficient transmission.
 */
void uart_tx(const char *cstring);

/**
 * @brief Send data buffer to the host.
 * @param data Pointer to the data buffer to send
 * @param len Length of the data buffer in bytes
 * 
 * Sends the specified number of bytes using DMA for efficient transmission.
 */
void uart_tx(const char *data, uint32_t len);

/**
 * @brief Poll for received UART data and process commands.
 * 
 * Checks for received data packets and processes any pending commands.
 * This function should be called regularly to handle incoming communication.
 */
void poll_uart();
