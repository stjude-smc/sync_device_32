/*
 * sd_comport.h
 *
 * Created: 9/5/2024 4:31:45 PM
 *  Author: rkiselev
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

// Data packet for serial communication
typedef struct DataPacket
{
	char     cmd[4];    // 3-character command, null-terminated
	uint32_t arg1;      // first argument for the command
	uint32_t arg2;      // second argument for the command
	uint32_t ts_us;     // timestamp for command execution in us
	uint32_t N;         // number of times to run command (0 = forever)
	uint32_t interv_us; // interval between command executions, in us
} DataPacket;

void init_uart_comm(void);

// Send data to the host
void uart_tx(const char *cstring);
void uart_tx(const char *data, uint32_t len);

void poll_uart();
