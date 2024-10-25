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

#include <queue>          // priority queue, FIFO queue

#ifndef UNIT_TEST
#include <asf.h>
#include "pins.h"
#endif

#include "globals.h"
#include "uart_comm.h"

// Default pulse duration, us
extern volatile uint32_t default_pulse_duration_us;

// An event function expects two uint32_t arguments
using EventFunc = void (*)(uint32_t, uint32_t);

typedef struct  __attribute__((packed)) Event
{
	EventFunc     func;		   // 4 bytes - pointer to a function to call
	uint32_t	  arg1;        // 4 bytes - first function argument
	uint32_t	  arg2;        // 4 bytes - second function argument
	union                      // 8 bytes - timestamp for function call
	{
		uint64_t  ts64_cts;
		struct {
			uint32_t ts_lo32_cts;  // lower 4 bytes - for timer/counter value
			uint32_t ts_hi32_cts;  // upper 4 bytes - for timer/counter overflows
		};
	};
	uint32_t	  N;           // 4 bytes - number of remaining calls
	uint32_t	  interv_cts;  // 4 bytes - interval between the calls

	bool operator<(const Event& other) const {
		return this->ts64_cts > other.ts64_cts;
	}
} Event;  // 28 bytes


// Scheduled events, sorted by timestamp
extern std::priority_queue<Event> event_queue;

extern volatile uint64_t sys_tc_ovf_count;

// Allocate memory and create an event from data packet
Event* event_from_datapacket(const DataPacket* packet, EventFunc func=nullptr);

void schedule_event(const Event *event, bool relative = true);

void schedule_pulse(const DataPacket *data, bool is_positive);
void schedule_pin(const DataPacket *data);
void schedule_toggle(const DataPacket *data);
void schedule_burst(const DataPacket *data);
void schedule_enable_pin(const DataPacket *data);
void schedule_disable_pin(const DataPacket *data);

void process_events();

void init_burst_timer();

void init_sys_timer();
void start_sys_timer();
void stop_sys_timer();
void pause_sys_timer();
extern volatile bool sys_timer_running;

inline uint64_t current_time_cts()
{
	return sys_tc_ovf_count | SYS_TC->TC_CHANNEL[SYS_TC_CH].TC_CV;
}
uint64_t current_time_us();

float current_time_s();

// Functions to use within Event structure
void tgl_pin_event_func(uint32_t arg1_pin_idx, uint32_t arg2);
void set_pin_event_func(uint32_t arg1_pin_idx, uint32_t arg2);
void start_burst_func  (uint32_t arg1_period,  uint32_t arg2_unused);
void stop_burst_func   (uint32_t arg1_unused,  uint32_t arg2_unused);
void enable_pin_func   (uint32_t arg1_pin_idx, uint32_t arg2);
void disable_pin_func  (uint32_t arg1_pin_idx, uint32_t arg2);

inline bool is_event_missed()
{
	static Event e;
	bool result = false;

	if (sys_timer_running && !event_queue.empty())
	{
		e = event_queue.top();
		result = current_time_cts() > (e.ts64_cts + TS_MISSED_TOLERANCE_CTS);
	}
	return result;
}
