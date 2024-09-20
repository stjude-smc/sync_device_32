/*
 * sd_comport.h
 *
 * Created: 9/5/2024 4:31:45 PM
 *  Author: rkiselev
 */ 

#pragma once
#include <asf.h>
#include "globals.h"

// Data packet for serial communication
typedef struct DataPacket
{
	char     cmd[4];  // 3-character command, null-terminated
	uint32_t arg1;
	uint32_t arg2;
	uint32_t timestamp;
	uint32_t N;
	uint32_t interval;
} DataPacket;

void init_uart_comm(void);

// Send data to the host
void sd_tx(const char *cstring);
void sd_tx(const char *buf, uint32_t len);
