/*
 * sd_events.c
 *
 * Created: 9/9/2024 5:09:35 PM
 *  Author: rkiselev
 */ 

#include <asf.h>
#include "sd_events.h"
#include "sd_triggers.h"
#include "sd_comport.h"

static inline void disable_event_irq(void)
{
	tc_disable_interrupt(SYS_TC, SYS_TC_CH, TC_IER_CPAS);
}
static inline void enable_event_irq(void)
{
	tc_enable_interrupt(SYS_TC, SYS_TC_CH, TC_IER_CPAS);
}

// return current system time
static inline uint32_t current_time(void)
{
	return tc_read_cv(SYS_TC, SYS_TC_CH);
}


Pulse pulse_table[10] = {0};
size_t pulse_table_n_items = 0;

Event event_table[MAX_N_EVENTS] = {0};
size_t event_table_size = 0;

int pulse_srt_pending_desc(const void* a, const void* b);
int pulse_srt_ts_asc(const void* a, const void* b);
int cmp_event_by_active(const void* a, const void* b);
int cmp_event_by_ts(const void* a, const void* b);

// Comparison functions
int pulse_srt_pending_desc(const void* a, const void* b)
{
	const Pulse* A = (const Pulse*) a;
	const Pulse* B = (const Pulse*) b;
	return B->pending - A->pending;
}

int pulse_srt_ts_asc(const void* a, const void* b)
{
	const Pulse* A = (const Pulse*) a;
	const Pulse* B = (const Pulse*) b;
	return A->timestamp - B->timestamp;
}

int cmp_event_by_active(const void* a, const void* b)
{
	const Event* A = (const Event*) a;
	const Event* B = (const Event*) b;
	return B->active - A->active;
}

int cmp_event_by_ts(const void* a, const void* b)
{
	const Event* A = (const Event*) a;
	const Event* B = (const Event*) b;
	return A->timestamp - B->timestamp;
}


// this takes about 25us
void update_pulse_table(void)
{
	disable_event_irq();
	
	// update size of the pulse_table
	qsort(pulse_table, 10, sizeof(pulse_table[0]), pulse_srt_pending_desc);
	for (int j = 0; j<=10; j++)
	{
		if (!(pulse_table[j].pending))
		{
			pulse_table_n_items = j;  // update number of elements
			break;
		}
	}
	// sort by timestamp
	qsort(pulse_table, pulse_table_n_items, sizeof(pulse_table[0]), pulse_srt_ts_asc);
	
	enable_event_irq();
}


// Purge any completed events, update table size
// Sort table by the order of occurrence
// This function blocks system timer interrupt to
// avoid data corruption
void update_event_table(void)
{
	disable_event_irq();
	
	// update size of the event_table
	qsort(event_table, event_table_size, sizeof(Event), cmp_event_by_active);
	for (int i = 0; i <= event_table_size; i++)
	{
		if (!(event_table[i].active))
		{
			event_table_size = i;  // update number of elements
			break;
		}
	}
	// sort by timestamp
	qsort(event_table, event_table_size, sizeof(Event), cmp_event_by_ts);

	// We might have missed	some events while sorting the table
	process_pending_events();
	
	// Re-enable the system timer RA compare interrupt
	enable_event_irq();	
}


// Process all events whose timestamps are current or have passed
void process_pending_events(void)
{
	disable_event_irq();
	
	// We iterate through the presorted table and fire all pending events
	for (uint32_t event_idx = 0; event_idx < event_table_size ; event_idx++)
	{
		Event *e = &event_table[event_idx];
		if (current_time() >= e->timestamp)
		{
			if (e->active)
			{
				e->func(e->arg1, e->arg2);
				// TODO: update event count and active status (see diagram). If event becomes inactive -> update table
				e->active = false; // TODO - at the moment all events are one-time events
			}
		}
		// ...until we find the first future event
		else
		{
			// Set interrupt for a future event
			tc_write_ra(SYS_TC, SYS_TC_CH, e->timestamp);
			break;
		}
	}

	enable_event_irq();
}


void schedule_event(Event e)
{
	// Check if the event should be fired immediately, and that's it
	if (e.timestamp <= 5)  // that is 5 us
	{
		e.func(e.arg1, e.arg2);
		return;
	}
	
	// Can we schedule it?
	if (event_table_size >= MAX_N_EVENTS)
	{
		sd_tx("ERR: event table is full!\n");
		return;
	}

	// Add event to the event table
	e.active = true;
	e.timestamp = us2cts(e.timestamp) + current_time();

	disable_event_irq();
	event_table[event_table_size++] = e;
	
	// Since we modified the event table, we have to update and sort it
	update_event_table();

	enable_event_irq();	
}


void send_pulse(ioport_pin_t pin, uint32_t duration)
{
	ioport_toggle_pin_level(pin);
	Pulse* p_pulse = &pulse_table[pulse_table_n_items];
	p_pulse->pending = true;
	p_pulse->pin = pin;
	p_pulse->timestamp = current_time() + duration;
	update_pulse_table();
}



void start_sys_timer(void)
{
	sysclk_enable_peripheral_clock(ID_SYS_TC);
	
	tc_init(SYS_TC, SYS_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK4 | TC_CMR_WAVE
	);
	
	// Enable the interrupt on register compare
	enable_event_irq();
	
	NVIC_EnableIRQ(SYS_TC_IRQn);
	
	tc_start(SYS_TC, SYS_TC_CH);
}

// This interrupt runs when current time reaches the timestamp of the
// element 0 in the event table. We might end up processing more than one
// event. At the end of the interrupt, we need to purge completed events
// from the table, sort it by timestamp, and schedule the next interrupt
void SYS_TC_Handler(void)
{
    // Read Timer Counter Status to clear the interrupt flag
	uint32_t status = tc_get_status(SYS_TC, SYS_TC_CH);
	
    if (status & TC_SR_CPAS) {  // RA match
		process_pending_events();
    }
}
