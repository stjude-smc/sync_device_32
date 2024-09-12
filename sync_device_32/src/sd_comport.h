/*
 * sd_comport.h
 *
 * Created: 9/5/2024 4:31:45 PM
 *  Author: rkiselev
 */ 

#pragma once
#include <asf.h>
#include <sd_globals.h>

// Laser shutter states - in active and idle mode
typedef struct
{
	uint8_t lasers_in_use;
	bool ALEX_enabled;
} LaserShutter;

// Data packet for serial communication
typedef struct Data
{
	uint8_t  cmd[4];      // size=4 null-terminated 3 chr command
	union {               // size=4
		uint32_t arg1;
		uint8_t pin[4];   // null-terminated 3 ch3 pin name
		};
	uint32_t arg2;        // size=4
	uint32_t timestamp;   // size=4
} Data;

void sd_init_UART(void);

// Send data to the host
void sd_tx(const char *cstring);
