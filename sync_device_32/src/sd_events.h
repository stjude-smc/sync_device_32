/*
 * sd_events.h
 *
 * Created: 9/9/2024 5:07:21 PM
 *  Author: rkiselev
 */ 


#pragma once

#include "sd_globals.h"



typedef struct Pulse
{
	uint32_t	   timestamp;
	ioport_pin_t   pin;
	uint32_t	   polarity;
	bool		   pending;
} Pulse;

extern Pulse pulse_table[10];

void start_ote_timer(void);
