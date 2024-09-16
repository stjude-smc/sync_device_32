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
	return sys_timer_running ? tc_read_cv(SYS_TC, SYS_TC_CH) : 0;
}



Event event_table[MAX_N_EVENTS] = {0};
Event event_table_copy[MAX_N_EVENTS] = {0};
volatile uint32_t event_table_start = 0;  // Pointer to the next pending event
volatile uint32_t event_table_end = 0;    // Pointer to the last pending event
volatile bool event_table_copy_valid = false;

bool sys_timer_running = 0;

int cmp_event_by_active(const void* a, const void* b);
int cmp_event_by_active(const void* a, const void* b)
{
	const Event* A = (const Event*) a;
	const Event* B = (const Event*) b;
	return B->active - A->active;
}

int cmp_event_by_ts(const void* a, const void* b);
int cmp_event_by_ts(const void* a, const void* b)
{
	const Event* A = (const Event*) a;
	const Event* B = (const Event*) b;
	return A->timestamp - B->timestamp;
}



// This function attempts to update the event table but may
// be interrupted by timer. In this case, it has to start over.

// Purge any completed events, update table size
// Sort table by the order of occurrence
// This function blocks system timer interrupt to
// avoid data corruption
void update_event_table(void)
{
	if (event_table_copy_valid)
		return;

	// Make a copy of the event table if necessary
	memcpy(event_table_copy, event_table, sizeof(Event)*event_table_end);
	event_table_copy_valid = true;
	
	// update size of the event_table
	size_t event_table_new_end = event_table_end;
	qsort(event_table_copy, event_table_end, sizeof(Event), cmp_event_by_active);
	for (uint32_t i = 0; i <= event_table_end; i++)
	{
		if (!(event_table_copy[i].active))
		{
			event_table_new_end = i;    // total number of pending elements
			break;
		}
	}
	// sort by timestamp
	qsort(event_table_copy, event_table_new_end, sizeof(Event), cmp_event_by_ts);


	disable_event_irq();    // prevent race condition
	if (event_table_copy_valid)
	{
		// Swap the original event table with it's copy
		event_table_start = 0;  // first element is the next pending event
		event_table_end = event_table_new_end;  // Update number of events
		memcpy(event_table, event_table_copy, sizeof(Event)*event_table_end);  // Swap the table
	}
	enable_event_irq();
}


// Process all events whose timestamps are current or have passed
void process_pending_events(void)
{
	disable_event_irq();
	
	// We iterate through the presorted table and fire all pending events
	for (uint32_t event_idx = event_table_start; event_idx < event_table_end; event_idx++)
	{
		Event *e = &event_table[event_idx];
		if (current_time() >= e->timestamp)
		{
			if (e->active)
			{
				e->func(e->arg1, e->arg2);
				event_table_copy_valid = false;
				// TODO: update event count and active status (see diagram). If event becomes inactive -> update table
				e->active = false; // TODO - at the moment all events are one-time events
			}
		}
		// ...until we find the first future event
		else
		{
			event_table_start = event_idx;
			// Set interrupt for a future event
			tc_write_ra(SYS_TC, SYS_TC_CH, e->timestamp);
			break;
		}
	}

	enable_event_irq();
}


void verify_ra_is_set(void)
{
	uint32_t new_RA = 0xffffffff;
	for (uint32_t event_idx = event_table_start; event_idx < event_table_end; event_idx++)
	{
		Event *e = &event_table[event_idx];
		if (e->active)
		{
			new_RA = min(new_RA, e->timestamp);
		}
    }
	tc_write_ra(SYS_TC, SYS_TC_CH, new_RA);
	
	if (current_time() > tc_read_ra(SYS_TC, SYS_TC_CH))
	{
		process_pending_events();
	}
}


void schedule_event_abs_time(const Event event)
{
	// Check if the event should be fired immediately, and that's it
	if (event.timestamp == 0)  // 1us is 0cts but is not executed immediately
	{
		event.func(event.arg1, event.arg2);
		return;
	}
	
	// Can we schedule it?
	if (event_table_end >= MAX_N_EVENTS)
	{
		sd_tx("ERR: event table is full!\n");
		return;
	}

	// Add event to the event table
	disable_event_irq();
	Event* p_event = &event_table[event_table_end++];
	memcpy(p_event, &event, sizeof(event));
	p_event->active = true;
	p_event->timestamp = us2cts(event.timestamp) + 1;
	
	// Since we modified the event table, we have to update and sort it
	event_table_copy_valid = false;
	update_event_table();
	
	enable_event_irq();
}

void schedule_event(const Event event)
{
	Event e = event;
	e.timestamp = event.timestamp + cts2us(current_time()) + 1;
	schedule_event_abs_time(e);
}

void _toggle_func(uint32_t pin_idx, uint32_t arg2);
void _toggle_func(uint32_t pin_idx, uint32_t arg2)
{
	ioport_toggle_pin_level(pin_idx);
}

void _set_pin_func(uint32_t pin_idx, uint32_t arg2);
void _set_pin_func(uint32_t pin_idx, uint32_t arg2)
{
	ioport_set_pin_level(pin_idx, arg2);
}

void schedule_pulse(const Data data)
{
	// Activate this output pin
	uint32_t pin_idx = pin_name_to_ioport_id(data.pin);
	ioport_set_pin_dir(pin_idx, IOPORT_DIR_OUTPUT);

	Event e = {0};
	// Schedule front of the pulse
	e.func = _toggle_func;
	e.arg1 = pin_idx;
	uint32_t now = cts2us(current_time());
	e.timestamp = data.timestamp + now;
	schedule_event_abs_time(e);
	
	// Schedule back of the pulse. arg2 is the pulse duration
	e.timestamp += data.arg2;
	schedule_event_abs_time(e);
}


void schedule_pin(const Data data)
{
	// Activate this output pin
	uint32_t pin_idx = pin_name_to_ioport_id(data.pin);
	ioport_set_pin_dir(pin_idx, IOPORT_DIR_OUTPUT);

	Event e = {0};
	// Schedule front of the pulse
	e.func = _set_pin_func;
	e.arg1 = pin_idx;
	e.arg2 = data.arg2;
	e.timestamp = data.timestamp;
	schedule_event(e);
}

void schedule_toggle(const Data data)
{
	// Activate this output pin
	uint32_t pin_idx = pin_name_to_ioport_id(data.pin);
	ioport_set_pin_dir(pin_idx, IOPORT_DIR_OUTPUT);

	Event e = {0};
	// Schedule front of the pulse
	e.func = _toggle_func;
	e.arg1 = pin_idx;
	e.timestamp = data.timestamp;
	schedule_event(e);
}


// Start timer from 0
void start_sys_timer(void)
{
	tc_start(SYS_TC, SYS_TC_CH);
	sys_timer_running = true;
}

void init_sys_timer(void)
{
	sysclk_enable_peripheral_clock(ID_SYS_TC);
	
	tc_init(SYS_TC, SYS_TC_CH,
			TC_CMR_TCCLKS_TIMER_CLOCK3 | TC_CMR_WAVE
	);
	
	// Enable the interrupt on register compare
	enable_event_irq();
	
	NVIC_EnableIRQ(SYS_TC_IRQn);
}

// This re-adjusts timestamps of scheduled events so we can pause and continue
void pause_sys_timer(void)
{
	tc_stop(SYS_TC, SYS_TC_CH);
	sys_timer_running = false;
	uint32_t stopped_at = tc_read_cv(SYS_TC, SYS_TC_CH);
	
	// update all pending events in the event table
	for (uint32_t event_idx = event_table_start; event_idx < event_table_end ; event_idx++)
	{
		event_table[event_idx].timestamp -= stopped_at;
	}
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
