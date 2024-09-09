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
	struct
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

	char bytes[5];
};

void sd_init_UART(void);

// Send data to the host
void sd_send_chr(const char chr);
void sd_send_string(const char *cstring);

// Retrieve data received from host. If no data received within 16ms, return ERR_TIMEOUT
errcode sd_rx_byte(uint8_t *byte);
errcode sd_rx_string(uint8_t *bytearray, uint8_t size);

// Retrieve and process a 5-byte data packet from host, if available
void poll_UART(void);