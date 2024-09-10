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
union Data
{
	struct __attribute__((packed)) // disable structure padding on 32-bit architecture
	{
		uint8_t cmd;

		// All members below share the same chunk of memory
		union
		{
			LaserShutter lasers;
			int32_t int32_value;
			uint32_t uint32_value;
		};
	};

	uint8_t bytes[5];
};

void sd_init_UART(void);

// Send data to the host
void sd_tx(const char *cstring);
