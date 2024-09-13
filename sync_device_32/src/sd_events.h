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



typedef struct Event
{
	void		  (*func)(uint32_t arg1, uint32_t arg2);
	uint32_t	  arg1;
	uint32_t	  arg2;
	uint32_t	  timestamp;
	uint32_t	  N;
	uint32_t	  interval;
	bool		  active;
} Event;
extern Event event_table[MAX_N_EVENTS];
extern size_t event_table_size;

void update_event_table(void);
void schedule_event(Event event);


void start_sys_timer(void);
void process_pending_events(void);
