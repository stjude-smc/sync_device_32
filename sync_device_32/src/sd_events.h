/*
 * sd_events.h
 *
 * Created: 9/9/2024 5:07:21 PM
 *  Author: rkiselev
 */ 


#pragma once

#include "sd_globals.h"
#include "sd_comport.h"
#include "sd_pin_map.h"


void schedule_pulse(const Data data);
void schedule_pin(const Data data);
void schedule_toggle(const Data data);


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
extern Event event_table_copy[MAX_N_EVENTS];
extern volatile bool event_table_copy_valid;

void update_event_table(void);
void schedule_event(const Event event);
void schedule_event_abs_time(const Event event);


extern bool sys_timer_running;
void start_sys_timer(void);
void init_sys_timer(void);
void pause_sys_timer(void);
void process_pending_events(void);

// Housekeeping - ensure that if there are pending events in the future
// the RA is set for system timer
void verify_ra_is_set(void);