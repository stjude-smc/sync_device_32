/*
 * sd_events.h
 *
 * Created: 9/9/2024 5:07:21 PM
 *  Author: rkiselev
 */ 


#pragma once

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <queue>          // priority queue

#include "globals.h"
#include "uart_comm.h"

// An event function expects two uint32_t arguments
using EventFunc = void (*)(uint32_t, uint32_t);

typedef struct Event
{
	EventFunc     func;		   // pointer to a function to call
	uint32_t	  arg1;        // first function argument
	uint32_t	  arg2;        // second function argument
	uint32_t	  ts_cts;      // timestamp for function call
	uint32_t	  N;           // number of remaining calls
	uint32_t	  interv_cts;  // interval between the calls

	bool operator<(const Event& other) const {
		return this->ts_cts > other.ts_cts;
	}
} Event;  // 24 bytes


extern Event event_table_OLD[MAX_N_EVENTS];
extern std::priority_queue<Event> event_queue;

void event_from_datapacket(const DataPacket* packet, Event* new_event);

void schedule_event(Event event);
void schedule_event_abs_time(Event event);

void schedule_pulse(DataPacket data, bool is_positive);
void schedule_pin(DataPacket data);
void schedule_toggle(DataPacket data);

void process_events();

void init_sys_timer();
void start_sys_timer();
void pause_sys_timer();

bool is_sys_timer_running();
uint32_t current_time_cts();
uint32_t current_time_us();

// Functions to use within Event structure
void tgl_pin_event_func(uint32_t pin_idx, uint32_t arg2);
void set_pin_event_func(uint32_t pin_idx, uint32_t arg2);
