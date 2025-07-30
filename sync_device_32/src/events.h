/**
 * @file events.h
 * @author Roman Kiselev (roman.kiselev@stjude.org)
 * @brief Event scheduling system for microsecond-precision timing control.
 * 
 * This module implements a priority queue-based event scheduler that provides
 * microsecond-precision timing control for microscope synchronization. The system
 * can handle up to 450 scheduled events with 64-bit timestamp precision.
 * 
 * @version 2.3.0
 * @date 2024-09-09
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

/**
 * @brief Default pulse duration in microseconds.
 * 
 * This value is used when no specific pulse duration is provided.
 * Can be modified at runtime via system properties.
 */
extern volatile uint32_t default_pulse_duration_us;

/**
 * @brief Function pointer type for event execution.
 * 
 * Event functions receive two 32-bit arguments and perform the actual
 * hardware control operations (pin setting, toggling, etc.).
 * 
 * @param arg1 First argument (typically pin index or duration)
 * @param arg2 Second argument (typically additional parameters)
 */
using EventFunc = void (*)(uint32_t, uint32_t);

/**
 * @brief Event structure for priority queue scheduling.
 * 
 * This structure represents a single scheduled event in the system.
 * Events are ordered by timestamp in a priority queue for precise execution.
 * The structure is packed to minimize memory usage.
 * 
 * @note Total size: 28 bytes
 */
typedef struct Event
{
	EventFunc     func;		   /**< Function pointer to execute (4 bytes) */
	uint32_t	  arg1;        /**< First function argument (4 bytes) */
	uint32_t	  arg2;        /**< Second function argument (4 bytes) */
	union                      /**< 64-bit timestamp in clock ticks (8 bytes) */
	{
		uint64_t  ts64_cts;    /**< Full 64-bit timestamp */
		struct {
			uint32_t ts_lo32_cts;  /**< Lower 32 bits - timer/counter value */
			uint32_t ts_hi32_cts;  /**< Upper 32 bits - timer/counter overflows */
		};
	};
	uint32_t	  N;           /**< Number of remaining executions (4 bytes) */
	uint32_t	  interv_cts;  /**< Interval between executions in clock ticks (4 bytes) */

   // Constructor
   Event() : func([](uint32_t, uint32_t) { printf("ERR: Event func not set!\n"); }),
	   arg1(0), arg2(0), ts64_cts(0), N(0), interv_cts(0) {}


	bool operator<(const Event& other) const {
		return this->ts64_cts > other.ts64_cts;
	}
} Event;  // 28 bytes


/**
 * @brief Priority queue of scheduled events.
 * 
 * Events are automatically sorted by timestamp (earliest first).
 * The queue is processed by the system timer interrupt handler.
 */
extern std::priority_queue<Event> event_queue;

/**
 * @brief System timer overflow counter.
 * 
 * Tracks the number of timer overflows to maintain 64-bit precision
 * across the full time range of the system.
 */
extern volatile uint64_t sys_tc_ovf_count;

/**
 * @brief Create an event from a data packet.
 * 
 * Allocates memory and initializes an Event structure from the provided
 * data packet. If no function is specified, a default error handler is used.
 * 
 * @param packet Pointer to the data packet containing event parameters
 * @param func Optional function pointer (defaults to nullptr)
 * @return Pointer to the allocated Event structure
 */
Event* event_from_datapacket(const DataPacket* packet, EventFunc func=nullptr);

/**
 * @brief Schedule an event for execution.
 * 
 * Adds an event to the priority queue for execution at the specified time.
 * 
 * @param event Pointer to the event to schedule
 * @param relative If true, timestamp is relative to current time (default: true)
 */
void schedule_event(const Event *event, bool relative = true);

void schedule_pulse(const DataPacket *data, bool is_positive);
void schedule_pulse(uint32_t pin_idx, uint32_t pulse_duration_us, uint64_t timestamp_us,
                    uint32_t N, uint32_t interval_us, bool relative = true);
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
