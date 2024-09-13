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
	bool		   pending;
} Pulse;

extern Pulse pulse_table[10];
extern size_t pulse_table_n_items;

void update_pulse_table(void);
void send_pulse(ioport_pin_t pin, uint32_t duration);

void start_ote_timer(void);
