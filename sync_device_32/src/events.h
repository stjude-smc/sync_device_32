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

#ifndef UNIT_TEST
#include <asf.h>
#include "pins.h"
#endif

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

// Allocate memory and create an event from data packet
Event* event_from_datapacket(const DataPacket* packet, EventFunc func=nullptr);

void schedule_event(const Event *event, bool relative = true);

void schedule_pulse(const DataPacket *data, bool is_positive);
void schedule_pin(const DataPacket *data);
void schedule_toggle(const DataPacket *data);

void process_events();
bool is_event_missed();
Event get_next_event(); // returns the next event, thread-safe

inline void update_ra();

void init_sys_timer();
void start_sys_timer();
void stop_sys_timer();
void pause_sys_timer();

extern volatile bool sys_timer_running;
//uint32_t current_time_cts();
inline uint64_t current_time_cts()
{
	return SYS_TC->TC_CHANNEL[SYS_TC_CH].TC_CV;
}
uint32_t current_time_us();

// Functions to use within Event structure
void tgl_pin_event_func(uint32_t pin_idx, uint32_t arg2);
void set_pin_event_func(uint32_t pin_idx, uint32_t arg2);
